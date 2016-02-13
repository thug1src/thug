/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Core Library											**
**																			**
**	Module:			Debug (DBG)		 										**
**																			**
**	File name:		p_debug.cpp												**
**																			**
**	Created by:		05/27/99	-	mjb										**
**																			**
**	Description:	Platform specific debug code							**
**																			**
*****************************************************************************/

#include <stdio.h>
#include <core/defines.h>
#include <core/support.h>
#include <core/debug.h>
#include <libsn.h>


#ifdef __NOPT_DEBUG__

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/


/*****************************************************************************
**								  Externals									**
*****************************************************************************/

extern char _dbg_start[];
extern char _dbg_end[];
/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Dbg
{

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#ifdef __NOPT_FULL_DEBUG__

enum
{
	vMAX_CLASS_NODES	= 28000,
	vMAX_INSTANCE_NODES	= 100000
};

static const int vFREE_CLASS_BLOCK_ID 	= 0x02030405;
static const int vFREE_INST_BLOCK_ID 	= 0x27354351;

/*****************************************************************************
**								Private Types								**
*****************************************************************************/
	
struct FreeNode
{
	FreeNode* 	next;
	int			id;

} ;

union ClassNodeBlock
{
	char			pad[sizeof(Spt::ClassNode)];
	FreeNode		free_node;

};
  
union InstanceNodeBlock
{
	char			pad[sizeof(Spt::ClassNode::InstanceNode)];
	FreeNode		free_node;

};

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

static ClassNodeBlock*		spClassNodeFreeList = NULL;
static InstanceNodeBlock*	spInstanceNodeFreeList = NULL;


/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

void		set_up( void )
{
	spClassNodeFreeList = (ClassNodeBlock*)_dbg_start;

	ClassNodeBlock* p_class_node = spClassNodeFreeList;

	for( int i = 0;  i < ( vMAX_CLASS_NODES - 1 ); i++ )
	{
		p_class_node->free_node.id = vFREE_CLASS_BLOCK_ID;
		p_class_node->free_node.next = (FreeNode*)(++p_class_node);
	}

	p_class_node->free_node.id = vFREE_CLASS_BLOCK_ID;
	(p_class_node++)->free_node.next = (FreeNode*)spClassNodeFreeList;


	spInstanceNodeFreeList = (InstanceNodeBlock*)(++p_class_node);
	InstanceNodeBlock* p_inst_node = spInstanceNodeFreeList;

	for( int i = 0; i < ( vMAX_INSTANCE_NODES - 1 ) ; i++ )
	{
		p_inst_node->free_node.id = vFREE_INST_BLOCK_ID;
		p_inst_node->free_node.next = (FreeNode*)(++p_inst_node);
	}
	p_inst_node->free_node.id = vFREE_INST_BLOCK_ID;
	(p_inst_node++)->free_node.next = (FreeNode*)spInstanceNodeFreeList;


	Dbg_MsgAssert( (int)p_inst_node <= (int)_dbg_end, 
		"Dbg Mem Block not big enough (%d bytes too small)", (int)p_inst_node - (int)_dbg_end );

	if ( (int)p_inst_node < ((int)_dbg_end))
	{
		Dbg_Warning ( "%dbytes unused in Dbg mem block",((int)_dbg_end - (int)p_inst_node));
	}

	Dbg_Notify ( "Dbg Mem Block Allocated successfully" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		close_down( void )
{
	int 				count 			= 0;	
	ClassNodeBlock* 	p_class_node 	= spClassNodeFreeList;	
	InstanceNodeBlock* 	p_inst_node 	= spInstanceNodeFreeList; 

	do
	{
		Dbg_MsgAssert ( p_class_node->free_node.id == vFREE_CLASS_BLOCK_ID, "Block not free" );
		p_class_node = (ClassNodeBlock*)p_class_node->free_node.next;
		count++;
	} while ( p_class_node != spClassNodeFreeList );

	Dbg_MsgAssert ( count == vMAX_CLASS_NODES,
		"%d Class Nodes still registered", vMAX_CLASS_NODES - count );

	count = 0;

	do
	{		
		Dbg_MsgAssert ( p_inst_node->free_node.id == vFREE_INST_BLOCK_ID, "Block not free" );
		p_inst_node = (InstanceNodeBlock*)p_inst_node->free_node.next;
		count++;
	} while ( p_inst_node != spInstanceNodeFreeList );

	Dbg_MsgAssert ( count == vMAX_INSTANCE_NODES,
		"%d Instance Nodes still registered", vMAX_INSTANCE_NODES - count );

	Dbg_Notify ( "Dbg Mem Block Released successfully" );
}

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/


void*		NewClassNode( size_t size )
{
	void* p_ret = (void*)spClassNodeFreeList;
	
	Dbg_MsgAssert ( (int)spClassNodeFreeList != (int)(spClassNodeFreeList->free_node.next), 
					"ClassNode pool full" );

	Dbg_MsgAssert ( spClassNodeFreeList->free_node.id == vFREE_CLASS_BLOCK_ID, "Not a Free Class Node" ); 

	spClassNodeFreeList = (ClassNodeBlock*)spClassNodeFreeList->free_node.next;
//	printf("NewClassNode %p  next %p\n",p_ret, spClassNodeFreeList);
	return p_ret;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		DeleteClassNode( void* pMem )
{
 	Dbg_MsgAssert ( (((int)pMem >= (int)_dbg_start ) && (((int)pMem < (int)_dbg_end ))),
					 "Memory not in Debug block (%p)",pMem );
	
	ClassNodeBlock* p_freeblock = (ClassNodeBlock*)pMem;
	p_freeblock->free_node.next = (FreeNode*)spClassNodeFreeList;
	spClassNodeFreeList = (ClassNodeBlock*)p_freeblock;

	spClassNodeFreeList->free_node.id = vFREE_CLASS_BLOCK_ID;


//	printf("DeleteClassNode %p  next %p\n",spClassNodeFreeList, p_freeblock->free_node.next);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void*		NewInstanceNode( size_t size )
{
	void* p_ret = (void*)spInstanceNodeFreeList;
	
	Dbg_MsgAssert ( (int)spInstanceNodeFreeList != (int)(spInstanceNodeFreeList->free_node.next), 
					"InstanceNode pool full" );

	Dbg_MsgAssert ( spInstanceNodeFreeList->free_node.id == vFREE_INST_BLOCK_ID, "Not a Free Instance Node" ); 

	spInstanceNodeFreeList = (InstanceNodeBlock*)spInstanceNodeFreeList->free_node.next;



//	printf("NewInstanceNode %p  next %p\n",p_ret, spInstanceNodeFreeList);
	return p_ret;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		DeleteInstanceNode( void* pMem )
{
 	Dbg_MsgAssert ( (((int)pMem >= (int)_dbg_start ) && (((int)pMem < (int)_dbg_end ))),
					 "Memory not in Debug block (%p)",pMem );

	InstanceNodeBlock* p_freeblock = (InstanceNodeBlock*)pMem;
	p_freeblock->free_node.next = (FreeNode*)spInstanceNodeFreeList;
	spInstanceNodeFreeList = (InstanceNodeBlock*)p_freeblock;

	spInstanceNodeFreeList->free_node.id = vFREE_INST_BLOCK_ID;


//	printf("DeleteInstanceNode %p  next %p\n",spInstanceNodeFreeList, p_freeblock->free_node.next);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#else // __NOPT_FULL_DEBUG__

void		set_up( void )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		close_down( void )
{
}

#endif // __NOPT_FULL_DEBUG__


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Dbg

#endif // __NOPT_DEBUG__

#ifdef	__NOPT_ASSERT__

namespace Dbg
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	default_print( char *text )
{
	std::printf( text );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	default_trap( char* message )
{
//	uint*	ptr = reinterpret_cast< uint* >( 0x00000001 );
//
//	*ptr = NULL;

	snPause();
}
} // namespace Dbg

#endif
