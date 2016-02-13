/*****************************************************************************
**																			**
**			              Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		System Library											**
**																			**
**	Module:			Memory (Mem) 											**
**																			**
**	File name:		memtest.cpp												**
**																			**
**	Created by:		03/20/00	-	mjb										**
**																			**
**	Description:	Memory test code										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <sys\mem\region.h>
#include "heap.h"
#include "pool.h"
#include "pile.h"

/*****************************************************************************
**							  DBG Information								**
*****************************************************************************/


namespace Mem
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

class  TestA  : public Spt::Class
{
	

public :
			TestA ( uint val = 999 ) : value (val ) { }
	uint	GetValue( void ) const  { return value; }

protected :

	uint	value;

};

class  TestB : public  TestA 
{
	

public :
			TestB ( uint val = 666 ) { value = val; }

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

void Test( void )
{
	

	Dbg_Message( "Testing Memory sub-system" );

	Dbg_Message ( "Allocate 1M region from Mem::Manager's default BottomUp Heap" );
	
	Mem::AllocRegion*	region = new Mem::AllocRegion( 1024 * 1024 );
	
	Dbg_Message ( "Create Bottom-Up Heap and Top-Down Pile within region" );

	Mem::Heap*			heap = new Mem::Heap( region, Mem::Allocator::vBOTTOM_UP );
	Mem::Pile*			pile = new Mem::Pile( region, Mem::Allocator::vTOP_DOWN );

	Dbg_Message ( "Set current allocator to Mem::Manager's default TopDown Heap" );

	Manager::sHandle().PushContext( Manager::sHandle().TopDownHeap() );

	Dbg_Message ( "Allocate another 1M region" );

	Mem::AllocRegion*	region2 = new Mem::AllocRegion( 1024 * 1024 );
	Dbg_Message ( "Create Bottom-Up Pool (16x1K slots) and Top-Down Heap within region" );

	Mem::Pool*			pool = new Mem::Pool( region2, 1024, 16 );
	Mem::Heap*			heap2 = new Mem::Heap( region2, Mem::Allocator::vTOP_DOWN );
	
	Manager::sHandle().PushContext( pile );

//	char*	test1( new char[100] );
//	Mem::Ptr<char>	test1( new char[100] );
	
	Mem::Ptr< TestB >	testB = new TestB[10];
						

	Dbg_Message ( "Test B = %p, value = %d", testB.Addr(), testB->GetValue() );
#if 0
	Mem::Manager::sInstance().Delete(testB.Addr());
	uint	a = testB->GetValue();
#endif


	
//	Mem::Ptr< TestA >	ctestA = testB;
//	Mem::PtrToConst< TestA >	ctestA = testB;
	




//	Mem::Ptr< TestA >	testA( testB );

//	Mem::Ptr< TestA >	testA;
//	testA = testB;

#if 0 // poor little XBOX - can't handleit!
	Mem::Ptr< TestA >	testA = testB;
	testA+=9;	 // 10 doesn't assert properly - why???
#endif

//	Mem::PtrToConst< TestA >	ctestA( testB );


//	Mem::PtrToConst< TestA >	ctestA;
//	ctestA = testB;
	
//	Mem::PtrToConst< TestA >	ctestA = testB;	 // gives warning
	
//	Mem::PtrToConst< TestB >	ctestB = testB;
//	Mem::PtrToConst< TestA >	ctestA = ctestB;	
	
	
//	ctestB = testB;
//	ctestB->GetValue();
//	TestB	ttestB = *ctestB++;
	
#if 0
	Mem::PtrToConst< TestB >	stestB1 = (ctestB + 1);

	Mem::PtrToConst< TestB >	stestB10 = (ctestB + 10);
#endif


//	Mem::Ptr< int >		intPtr = new int;
//	int		b = *intPtr;

	char*	test2[17];

	for( uint i = 0; i < 16; i++ )
	{
		test2[i] = new (pool) char[1024];
		Dbg_Message ( "%x  should be valid address", test2[i] );
	}

	test2[16] = new (pool, false) char[1024];
	Dbg_Message ( " %x  should be NULL address ( no free slots )", test2[16] ); 

	delete test2[0];
	delete test2[1];

	char* test3 = new char;
	
	Manager::sHandle().PushContext( heap );

	char* test4 = new char;

	Manager::sHandle().PushContext( heap2 );
	
	Manager::sHandle().PopContext();
	Manager::sHandle().PopContext();
	Manager::sHandle().PopContext();

	delete test3;
	delete test4;
//	delete test4;

	delete heap;
	delete pile;
	delete pool;
	delete heap2;


/*
	char*  tmp1 = new char;
	int*  tmp2 = new int;
	short*  tmp3 = new short;
	delete tmp1;
	delete tmp3;
	delete tmp2;
*/

	TestA*	testa = new TestA;
	Mem::Handle< TestA > test_handle = testa;

	TestA*	gooda = test_handle.GetPointer();
	Dbg_AssertType( testa, TestA );
	
	Dbg_MsgAssert(( gooda == testa ),( "Handles pointer not valid" ));

	if( gooda != testa )
	{
		printf("Error!!!!!   Handles pointer not valid\n" );
	}

	Dbg_AssertType( gooda, TestA );

	delete testa;
//	testa = new TestA;

	gooda = test_handle.GetPointer();	
	Dbg_MsgAssert(( gooda == NULL ),( "Handles pointer not NULL" ));

	if( gooda )
	{
		printf("Error!!!!!   Handles pointer not NULL\n" );
	}

	Dbg_MsgAssert(( false ),("DONE" ));
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Mem

