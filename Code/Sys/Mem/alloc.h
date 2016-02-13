/*****************************************************************************
**																			**
**			              Neversoft Entertainment	                        **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Sys Library												**
**																			**
**	Module:			Memory Manager (Mem)									**
**																			**
**	Created:		03/20/00	-	mjb										**
**																			**
**	File name:		core/sys/mem/alloc.h									**
**																			**
*****************************************************************************/

#ifndef	__SYS_MEM_ALLOC_H
#define	__SYS_MEM_ALLOC_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/support.h>
#include <sys\mem\region.h>
#include <sys/mem/memdbg.h>

#ifdef __PLAT_NGC__
#define __EFFICIENT__		// Comment in for efficient allocation.
#endif		// __PLAT_NGC__

//#ifdef	__PLAT_NGPS__											  
#define	__LINKED_LIST_HEAP__    
//#else
//#ifdef __NOPT_DEBUG__
//#define	__LINKED_LIST_HEAP__    
//#endif
//#endif

// Mick: on the CD build, we don't need any memory tracking
// this saves us at least 300K as we have over 30,000 individual allocations... crazy
// K: This used to be #ifdef __NOPT_CDROM__, changed cos the nopt_cdrom define is now gone.
#ifndef	__NOPT_ASSERT__
#undef	__LINKED_LIST_HEAP__
#endif

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace Mem
{


const	uint	vALLOC_MAGIC = 0xCF801FE1;			// just a random magic number, to indicate a block is valid

class	CMemProfile;


/*****************************************************************************
**							     Type Defines								**
*****************************************************************************/

class Region;

class  Record  : public Spt::Class
{
	

public:
			Record( void ) : m_count(0), m_peak(0) {}

			int		m_count;
			int		m_peak;

			Record&		operator+=( int src );
			Record&		operator-=( int src );

	const 	Record		operator++( int src );
	const 	Record		operator--( int src );
			Record&		operator++( void );
			Record&		operator--( void );

};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  Allocator  : public Spt::Class				// abstract base class 			
{
	

	friend class Manager;
	friend class Region;

public :

	enum Direction
	{		
		vBOTTOM_UP	= +1,
		vTOP_DOWN	= -1
	};

	static		uint 			sGetId( void* pAddr );
		
				Region*			ParentRegion( void );

				Record			mUsedBlocks;
				Record			mFreeBlocks;
				Record			mUsedMem;
				Record			mFreeMem;


	virtual		void			PushContext( void );
	virtual		void			PopContext( void );

#ifdef __EFFICIENT__
				void			PushAlign( int align );
				void			PopAlign( void );
				uint			GetAlign( void );
#endif		// __EFFICIENT__

#ifdef __NOPT_DEBUG__
				void			dump_free_list ( void );
				void			dump_used_list ( void );
#endif

				char *	GetName() {return mp_name;}
				Direction GetDirection() {return m_dir;}

/// sorry, need to access this from elsewhere, for debugging... (MICK)
//protected :

	class  BlockHeader  //: public Spt::Class
	{
		
	
	public :

#ifdef __NOPT_MEM_DEBUG__

				void				DbgAllocateBlock( void );
				void				DbgFreeBlock( void );
				void				DbgDump( void );

#endif // __NOPT_MEM_DEBUG__

									BlockHeader( Allocator* pAlloc, size_t size );
									~BlockHeader( void );

		static	BlockHeader* 		sRead( void* pMem );
		
				union
				{
					Allocator*		mpAlloc;
					BlockHeader*	mpNext;
				};

#ifndef __EFFICIENT__
				uint				mSize;
#endif
				uint				mId;
#ifdef __EFFICIENT__
#ifdef __NOPT_ASSERT__
				CMemProfile	*		mp_profile;
#endif		// __NOPT_ASSERT__
#else
				CMemProfile	*		mp_profile;
#endif __EFFICIENT__


		static const uint			sSize; // Aligned Block Header size


#ifdef	__LINKED_LIST_HEAP__    

				BlockHeader*		mp_prev_used;
				BlockHeader*		mp_next_used;
				
				void *				mp_debug_data;		// added for tracking callstacks for each allocation

#endif // __NOPT_DEBUG__

#ifdef __EFFICIENT__
				uint				mSize:24;
				uint				mPadBytes:8;		// Must be last byte in structure.
#endif
	};

#ifdef __LINKED_LIST_HEAP__
				void * 				find_block(void *p);
				BlockHeader* 		first_block();
#endif
	
    
	class  Context  : public Spt::Class
	{
		
	public:
								Context( void );
								~Context( void );
	
		Lst::Node< Context >	m_node;
		BlockHeader*			mp_free_list;
	
	#ifdef	__LINKED_LIST_HEAP__    
		BlockHeader*			mp_used_list;
	#endif
	
	};

									Allocator( Region* region, Direction dir, char *p_name );
	virtual							~Allocator( void );
	
			Region*					mp_region;
			Direction				m_dir;
			void*					mp_top;
			Context*				mp_context;

			char *					mp_name;		// debugging name, for checking

			Lst::Head< Context >	m_context_stack;

private :

	static  bool 			s_valid( void* pAddr );
	static	size_t			s_get_alloc_size( void* pAddr );
	static	Allocator* 		s_free( void* pAddr );
	static	void 			s_set_id( void* pAddr );
	virtual void*			allocate( size_t size, bool assert_on_fail ) = 0;
	virtual int				available(  ) {Dbg_MsgAssert(0,("available() not defined for this allocator!")); return 0;}
	virtual void*			reallocate_down( size_t newSize, void* pOld ) {Dbg_MsgAssert(0,("reallocate_down not defined for this allocator!")); return NULL;}
	virtual void*			reallocate_up( size_t newSize, void* pOld ) {Dbg_MsgAssert(0,("reallocate_up not defined for this allocator!")); return NULL;}
	virtual void*			reallocate_shrink( size_t newSize, void* pOld ) {Dbg_MsgAssert(0,("reallocate_shrink not defined for this allocator!")); return NULL;}
	virtual	void			free( BlockHeader* pAddr ) = 0;

	static	uint			s_current_id;
			Context			m_initial_context;
#ifdef __EFFICIENT__
	static uint				s_align_stack[16];
	static uint				s_align_stack_index;
#endif		// __EFFICIENT__
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

inline Record& Record::operator++( void )			// prefix
{
	

	*this += 1;
	return	*this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Record Record::operator++( int )	// postfix
{
	
	
	Record ret = *this;
	++( *this );

	return ret;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Record& Record::operator--( void )		// prefix
{
	

	*this -= 1;
	return	*this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline const Record Record::operator--( int )	// postfix
{
	
	
	Record ret = *this;
	--( *this );

	return ret;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Record& Record::operator+=( int src )
{
	

	m_count += src;
	
	if( m_count > m_peak )
	{
		m_peak = m_count;
	}
	
	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Record& Record::operator-=( int src )
{
	
	
	m_count -= src;

	Dbg_MsgAssert(( m_count >= 0 ), ( "Count Underflow" ));
	
	return *this;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Region* Allocator::ParentRegion( void )
{
	

	return mp_region; 
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef __EFFICIENT__
inline Allocator::BlockHeader::BlockHeader( Allocator* pAlloc, size_t size )
: mpAlloc( pAlloc ), mSize( size ), mPadBytes( 0 )
#else
inline Allocator::BlockHeader::BlockHeader( Allocator* pAlloc, size_t size )
: mpAlloc( pAlloc ), mSize( size )
#endif	// __EFFICIENT__
{
	

#ifdef	__LINKED_LIST_HEAP__    

	mp_next_used = NULL;
	mp_prev_used = NULL;
	mp_debug_data  = NULL;

#endif
#ifdef __NOPT_ASSERT__
	mp_profile = NULL;
#endif		// __NOPT_ASSERT__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline Allocator::BlockHeader::~BlockHeader( void )
{
	

#ifdef	__LINKED_LIST_HEAP__    
	Dbg_MsgAssert( mp_prev_used == NULL, ( "internal error" ));
	Dbg_MsgAssert( mp_next_used == NULL, ( "internal error" ));
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef __NOPT_MEM_DEBUG__


inline void	Allocator::BlockHeader::DbgAllocateBlock( void )
{	
	

	uint64* ptr = (uint64*)((uint)this + sSize );

	for ( uint i = 0; i < mSize; i += 8 )
	{
		*ptr++ = vALLOCATED_BLOCK;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void	Allocator::BlockHeader::DbgFreeBlock( void )
{
	
	
	uint64* ptr = (uint64*)((uint)this + sSize ); 	

	for ( uint i = 0; i < mSize; i += 8 )
	{
		*ptr++ = vFREE_BLOCK;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void	Allocator::BlockHeader::DbgDump( void )
{
	
#ifdef __NOPT_FULL_DEBUG__
	Dbg_Message ( "\nBlockHeader [%p]\nmpAlloc/Next:%p\nSize %d\nstamp %x\n", this, mpAlloc, mSize, classStamp );
#else
	Dbg_Message ( "\nBlockHeader [%p]\nmpAlloc/Next:%p\nSize %d\n", this, mpAlloc, mSize );
#endif
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#endif // __NOPT_MEM_DEBUG__

bool  SameContext(void *p_mem,  Allocator *p_alloc);


} // namespace Mem

#endif  // __SYS_MEM_ALLOC_H
