/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			TrickObject (OBJ)										**
**																			**
**	File name:		TrickObject.cpp											**
**																			**
**	Created by:		03/05/01	-	gj										**
**																			**
**	Description:	trick object code										**
**																			**
*****************************************************************************/

#ifndef __TRICK_OBJECT_H
#define __TRICK_OBJECT_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <core/list/node.h>
#include <core/hashtable.h>

/*****************************************************************************
**							Forward Declarations							**
*****************************************************************************/

namespace Script
{
	class CStruct;
}
	
namespace Obj
{

    class CTrickObjectManager;
    class CTrickCluster;

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

// this is the interface through which we will
// modify a specific world sector's colors
class  CTrickObject : public  Lst::Node< CTrickObject > 
{
	

	public:
		friend class	CTrickCluster;

	public:
		CTrickObject( uint32 name_checksum );
		bool		InitializeTrickObjectColor( int seqIndex );
		bool		ModulateTrickObjectColor( int seqIndex );
		bool		ClearTrickObjectColor( int seqIndex );

	protected:
		uint32		m_NameChecksum;

	private:
		CTrickObject( void );
};

// this represents a group of linked trick objects
// when the score contained in the cluster is beaten
// all of the trick objects will change color
class  CTrickCluster : public  Lst::Node< CTrickCluster > 
{
	

	public:
		friend class	CTrickObjectManager;
		friend void trick_off_object( CTrickCluster*, void* );
		friend void init_graffiti_state( CTrickCluster*, void* );

	public:
		CTrickCluster( uint32 name_checksum );
		virtual			~CTrickCluster();
		int				GetScore( uint32 skater_id );
		uint32			GetNameChecksum() { return m_NameChecksum; }
		bool			AddTrickObject( uint32 name_checksum );
		bool			Reset();
		bool			ModulateTrickObjectColor( int seqIndex );
		bool			FreeCluster( uint32 skater_id );
		bool			ClearCluster( int seqIndex );

	protected:
		CTrickObject*	find_trick_object( uint32 name_checksum );

	protected:
		uint32			m_NameChecksum;
		uint32			m_Score;
		uint32			m_OwnerId;
		bool			m_IsOwned;

	// list of objects that get colored when one of the cluster get triggered
		Lst::Head< CTrickObject >		m_TrickObjectList;

	private:
		CTrickCluster( void );
};

class  CTrickObjectManager  : public Spt::Class
{
	

	public :
		CTrickObjectManager( void );
		virtual			~CTrickObjectManager( void );

	public:
		int				GetScore( uint32 skater_id );
		bool			AddTrickCluster( uint32 name_checksum );
		bool			AddTrickObjectToCluster( uint32 name_checksum, uint32 cluster_checksum );
		bool			AddTrickAlias( uint32 alias_checksum, uint32 cluster_checksum );
		bool			DeleteAllTrickObjects();	// wipes out all the trick objects
		bool			ResetAllTrickObjects();		// resets all points to 0
		void			PrintContents();
		bool			ModulateTrickObjectColor( uint32 name_checksum, int skater_id );
		uint32			RequestLogTrick( uint32 num_pending_tricks, uint32* p_pending_tricks, char* p_inform_prev_owner, int skater_id, uint32 score );
		uint32			TrickObjectExists( uint32 name_checksum );
		bool			FreeTrickObjects( uint32 skater_id );
		void			TrickOffAllObjects( uint32 skater_id );
		uint8			GetCompressedTrickObjectIndex( uint32 name_checksum );
		uint32			GetUncompressedTrickObjectChecksum( uint8 compressed_index );
		uint32			SetInitGraffitiStateMessage( void* pMsg );
		void			SetObserverGraffitiState( Script::CStruct* pScriptStructure );
		void			ApplyObserverGraffitiState( void );
		CTrickCluster*	GetTrickCluster( uint32 name_checksum );

		int 			m_NumTrickClusters;
	protected:
		CTrickCluster*	get_aliased_cluster( uint32 alias_checksum );
		CTrickCluster*	find_trick_cluster( uint32 name_checksum );
		bool			clear_trick_clusters( int seqIndex );		
		
	protected:
		// full list of clusters
		Lst::HashTable< CTrickCluster >	m_TrickClusterList;

		// list of checksums that will fire off this cluster
		Lst::HashTable< CTrickCluster >	m_TrickAliasList;

		Script::CStruct* mp_ObserverState;
};

// per-skater
class  CPendingTricks  : public Spt::Class
{
	public:
		enum
		{
			vMAX_PENDING_TRICKS = 512,		// max number of items we can trick off in a combo
			// for now this number must be less or equal to the buffer size in gamemsg.h
		};

		CPendingTricks( void );
		bool		TrickOffObject( uint32 checksum );
		uint32		WriteToBuffer( uint32* p_buffer, uint32 max_size );
		bool		FlushTricks( void );
		uint32		m_Checksum[vMAX_PENDING_TRICKS];
		uint32		m_NumTrickItems;

	protected:

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

} // namespace Obj

#endif	// __TRICK_OBJECT_H
