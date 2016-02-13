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

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sk/objects/trickobject.h>

#include <gel/net/server/netserv.h>
#include <gel/scripting/array.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>

#include <gfx/gfxutils.h>
#include <gfx/nx.h>
#include <gfx/nxsector.h>

#include <sk/gamenet/gamenet.h>
#include <sk/modules/skate/goalmanager.h>
#include <sk/modules/skate/skate.h>
#include <sk/objects/skater.h>		   // just needed in mygetassociatednetworkconnection

#include <sys/config/config.h>

#include <sk/components/RailEditorComponent.h>

#ifdef __NOPT_ASSERT__	
//#include <sk/ParkEditor/ParkEd.h>
#endif

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

DefinePoolableClass(Lst::HashItem<Obj::CTrickCluster>);


enum
{
	vMAX_TRICKS_TO_FREE = 128
};

/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/

namespace Obj
{



/*****************************************************************************
**								Private Types								**
*****************************************************************************/
   
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

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTrickObject::CTrickObject( void ) : Lst::Node< CTrickObject >(this)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTrickObject::CTrickObject( uint32 name_checksum ) : Lst::Node< CTrickObject >(this)
{
	m_NameChecksum = name_checksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickObject::InitializeTrickObjectColor( int seqIndex )
{
	// Garrett: shouldn't need to do anything except clear color here
	// checks for the wibbling data, and creates
	// it if it doesn't already exist

	// TODO:  Should also screen out if it doesn't have any geometry
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(m_NameChecksum);				
	Dbg_MsgAssert(p_sector,("sGetSector(0x%x) returned NULL (%s)",m_NameChecksum,Script::FindChecksumName(m_NameChecksum)));
	p_sector->ClearColor();
	if (!Config::CD())
	{
		if ( Script::GetInt( "show_all_trick_objects", false ) )
		{
			// quick way to see all the trick objects in the scene
			ModulateTrickObjectColor( 2 );
		}
	}	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickObject::ModulateTrickObjectColor( int seqIndex )
{
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(m_NameChecksum);				
	Dbg_MsgAssert(p_sector,("sGetSector(0x%x) returned NULL (%s)",m_NameChecksum,Script::FindChecksumName(m_NameChecksum)));

	Script::CArray* p_graffiti_col_tab = Script::GetArray( "graffitiColors" );
	Dbg_MsgAssert( (uint) seqIndex < p_graffiti_col_tab->GetSize(), ( "graffitiColors array too small" ) );

	Script::CArray *p_entry = p_graffiti_col_tab->GetArray(seqIndex);
	
	#ifdef	__NOPT_ASSERT__
	int size = p_entry->GetSize();
	Dbg_MsgAssert(size >= 3 && size <= 4, ("wrong size %d for color array", size));
	#endif
	
	Image::RGBA color;

	color.r = p_entry->GetInteger( 0 );
	color.g = p_entry->GetInteger( 1 );
	color.b = p_entry->GetInteger( 2 );
	color.a = 128;

	p_sector->SetColor(color);
	return true;

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickObject::ClearTrickObjectColor( int seqIndex )
{
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(m_NameChecksum);				
	Dbg_MsgAssert(p_sector,("sGetSector(0x%x) returned NULL (%s)",m_NameChecksum,Script::FindChecksumName(m_NameChecksum)));

	Script::CArray* p_graffiti_col_tab = Script::GetArray( "graffitiColors" );
	Dbg_MsgAssert( (uint) seqIndex < p_graffiti_col_tab->GetSize(), ( "graffitiColors array too small" ) );

	Script::CArray *p_entry = p_graffiti_col_tab->GetArray(seqIndex);
	
	#ifdef	__NOPT_ASSERT__
	int size = p_entry->GetSize();
	Dbg_MsgAssert(size >= 3 && size <= 4, ("wrong size %d for color array", size));
	#endif

	Image::RGBA color;

	color.r = p_entry->GetInteger( 0 );
	color.g = p_entry->GetInteger( 1 );
	color.b = p_entry->GetInteger( 2 );
	color.a = 128;

	Image::RGBA orig_color = p_sector->GetColor();
	if ((orig_color.r == color.r) &&
		(orig_color.g == color.g) &&
		(orig_color.b == color.b))
	{
		p_sector->ClearColor();
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTrickCluster::CTrickCluster( void ) : Lst::Node< CTrickCluster >(this)
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTrickCluster::CTrickCluster( uint32 name_checksum ) : Lst::Node< CTrickCluster >(this)
{
	

	m_NameChecksum = name_checksum;

	Reset();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTrickCluster::~CTrickCluster( void )
{
	

	uint32 count = m_TrickObjectList.CountItems();

	// first iterate through all the items,
	// and destroy the associated objects
	for ( uint32 i = 0; i < count; i++ )
	{
		Lst::Node< CTrickObject >* pNode = m_TrickObjectList.GetItem( 0 );
		Dbg_Assert( pNode );
		CTrickObject* pObject = pNode->GetData();
		Dbg_Assert( pObject );
		delete pObject;
	}

	// now remove all the nodes from the list
	m_TrickObjectList.RemoveAllNodes();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CTrickCluster::GetScore( uint32 skater_id )
{
	return ( m_IsOwned && (skater_id == m_OwnerId) );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickCluster::Reset( void )
{
	

	m_Score = 0;
	m_OwnerId = 0;
	m_IsOwned = false;	// no owner
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	CTrickCluster::ModulateTrickObjectColor( int seqIndex )
{
	

	uint32 count = m_TrickObjectList.CountItems();

	for ( uint32 i = 0; i < count; i++ )
	{
		Lst::Node< CTrickObject >* pNode = m_TrickObjectList.GetItem( i );
		Dbg_Assert( pNode );
		CTrickObject* pObject = pNode->GetData();
		Dbg_Assert( pObject );
		pObject->ModulateTrickObjectColor( seqIndex );
	}

	// If playing a created park, update any created rails for this cluster.
	Mdl::Skate * p_skate_mod =  Mdl::Skate::Instance();
	if (p_skate_mod->m_cur_level == CRCD(0xb664035d,"load_sk5ed_gameplay"))
	{
		Obj::GetRailEditor()->ModulateRailColor(m_NameChecksum, seqIndex);
	}	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTrickObject* CTrickCluster::find_trick_object( uint32 name_checksum )
{
	

	uint32 count = m_TrickObjectList.CountItems();

	// find the appropriate trick object
	for ( uint32 i = 0; i < count; i++ )
	{
		Lst::Node< CTrickObject >* pNode = m_TrickObjectList.GetItem( i );
		Dbg_Assert( pNode );
		CTrickObject* pObject = pNode->GetData();
		Dbg_Assert( pObject );

		if ( pObject->m_NameChecksum == name_checksum )
		{
			return pObject;
		}
	}

	return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickCluster::AddTrickObject( uint32 name_checksum )
{
	

	Dbg_MsgAssert( !find_trick_object( name_checksum ),( "Trick object %s is already in list", Script::FindChecksumName(name_checksum) ));
	
	CTrickObject* pObject = new CTrickObject( name_checksum );

	if ( !pObject )
	{
		return false;
	}

	if ( pObject->InitializeTrickObjectColor( name_checksum ) )
	{
//		printf( "Adding %s to trick object list\n", Script::FindChecksumName( name_checksum ) );
		m_TrickObjectList.AddToTail( pObject );
		return true;
	}
	else
	{
		// if not valid
		delete pObject;
		return false;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTrickObjectManager::CTrickObjectManager( void ) : m_TrickClusterList(8), m_TrickAliasList(8)
{
//	m_TrickAliasCount = 0;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());

	mp_ObserverState = new Script::CScriptStructure;

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTrickObjectManager::~CTrickObjectManager( void )
{
	this->DeleteAllTrickObjects();
}

struct STrickObjectFreeInfo
{
	uint32	skater_id;			// which objects to free
	uint32	num_tricks;			// num tricks to free
	uint32	trick_buffer[vMAX_TRICKS_TO_FREE];
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickCluster::FreeCluster( uint32 skater_id )
{
	if ( this->m_IsOwned && ( this->m_OwnerId == skater_id ) )
	{
		this->m_IsOwned = false;
		this->m_Score = 0;
		return true;
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickCluster::ClearCluster( int seqIndex )
{

	uint32 count = m_TrickObjectList.CountItems();

	// find the appropriate trick object
	for ( uint32 i = 0; i < count; i++ )
	{
		Lst::Node< CTrickObject >* pNode = m_TrickObjectList.GetItem( i );
		Dbg_Assert( pNode );
		CTrickObject* p_object = pNode->GetData();
		Dbg_Assert( p_object );

		if (!p_object->ClearTrickObjectColor(seqIndex))
		{
			return false;
		}
	}
	
	// If playing a created park, clear any created rail colors for this cluster.
	Mdl::Skate * p_skate_mod =  Mdl::Skate::Instance();
	if (p_skate_mod->m_cur_level == CRCD(0xb664035d,"load_sk5ed_gameplay"))
	{
		Obj::GetRailEditor()->ClearRailColor(m_NameChecksum);
	}
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void free_trick_cluster(CTrickCluster* pCluster, void* pData)
{	
	Dbg_AssertPtr(pCluster);
	Dbg_AssertPtr(pData);

	STrickObjectFreeInfo* pFreeInfo = (STrickObjectFreeInfo*)pData;

	if ( pCluster->FreeCluster( pFreeInfo->skater_id ) )
	{
		Dbg_MsgAssert( pFreeInfo->num_tricks < vMAX_TRICKS_TO_FREE, ( "Too many trick objects to free" ) );
		pFreeInfo->trick_buffer[pFreeInfo->num_tricks] = pCluster->GetNameChecksum();		
		pFreeInfo->num_tricks++;

#ifdef __USER_GARY__
		Dbg_Message( "Freeing up %s\n", Script::FindChecksumName(pCluster->GetNameChecksum()) );
#endif
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickObjectManager::FreeTrickObjects( uint32 skater_id )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
    
	// clear the observer state when you exit the game
	printf( "Clearing observer state\n" );
	mp_ObserverState->Clear();
	
    if( gamenet_man->OnServer())
	{
		STrickObjectFreeInfo theFreeInfo;
		theFreeInfo.skater_id = skater_id;
		theFreeInfo.num_tricks = 0;

		m_TrickClusterList.HandleCallback(free_trick_cluster, (void*)&theFreeInfo);

		if ( theFreeInfo.num_tricks <= 0 )
		{
			// no tricks to free
			return false;
		}

		// send score updates, as something has changed
		Mdl::Skate * skate_mod = Mdl::Skate::Instance();
		skate_mod->SendScoreUpdates( false );
	}
    
	clear_trick_clusters(skater_id + 1);

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CTrickObjectManager::TrickObjectExists( uint32 name_checksum ){
	// gets cluster checksum
	CTrickCluster* pCluster = get_aliased_cluster( name_checksum );
	if ( pCluster )
	{
		return pCluster->m_NameChecksum;
	}
	else
	{
		return 0;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CTrickObjectManager::RequestLogTrick( uint32 num_pending_tricks, uint32* p_pending_tricks, char* p_inform_prev_owner, int skater_id, uint32 score )
{
    printf("RequestLogTrick called\n");

// GJ:  Note that this function only logs the trick;  a message
// will be sent out to the clients to actually modify the color

	uint32 i;

	Dbg_Assert( p_pending_tricks );
	Dbg_Assert( p_inform_prev_owner );

	Dbg_Assert( num_pending_tricks < 128 );
	static char s_change_mask[128];
	for ( i = 0; i < num_pending_tricks; i++ )
	{
		s_change_mask[i] = 0;
	}

	for ( i = 0; i < num_pending_tricks; i++ )
	{
		CTrickCluster* pCluster = get_aliased_cluster( p_pending_tricks[i] );

		// change only objects marked as trick objects
		if ( !pCluster )
			continue;

		// tell the goal manager
		Game::CGoalManager* pGoalManager = Game::GetGoalManager();
		Dbg_Assert( pGoalManager );
		pGoalManager->GotTrickObject( pCluster->GetNameChecksum(), score );

		// if the score beats the score
		if ( score > pCluster->m_Score )
		{
            bool owner_changed = ( pCluster->m_OwnerId != (uint32)skater_id );
			
			if ( !pCluster->m_IsOwned || owner_changed )
			{
				// that the trick object has changed states
				s_change_mask[i] = 1;
			}
			
			if ( pCluster->m_IsOwned && owner_changed )
			{
				// for tracking steal messages
				Dbg_Assert( pCluster->m_OwnerId >= 0 && pCluster->m_OwnerId < GameNet::vMAX_PLAYERS );
				p_inform_prev_owner[pCluster->m_OwnerId] = 1;
			}

			pCluster->m_OwnerId = skater_id;
			pCluster->m_Score = score;
			pCluster->m_IsOwned = true;

/*            Game::CGoalManager* p_GoalManager = Game::GetGoalManager();
            if ( p_GoalManager->IsInGraffitiGoal() )
            {
                p_GoalManager->GotGraffitiCluster( pCluster->GetNameChecksum() );
                // printf("tricked in %s\n", Script::FindChecksumName( pCluster->GetNameChecksum() ) );
            }
*/
		}
	}

	uint32 num_changed = 0;
	
	// shift them so that the changed ones are at the beginning
	for ( i = 0; i < num_pending_tricks; i++ )
	{
		if ( s_change_mask[i] )
		{
			p_pending_tricks[num_changed] = p_pending_tricks[i];
			num_changed++;
		}
	}
	
	return num_changed;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTrickCluster* CTrickObjectManager::GetTrickCluster( uint32 name_checksum )
{
	return find_trick_cluster( name_checksum );
}

struct SAutoTrick
{
	uint32	pPendingTricks[32];
	uint32	numPendingTricks;
};

Net::Conn* MyGetAssociatedNetworkConnection( uint32 skater_id )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Obj::CSkater* skater;
	GameNet::PlayerInfo* player;

	skater = skate_mod->GetSkaterById( skater_id );
	if( skater )
	{
		player = gamenet_man->GetPlayerByObjectID( skater->GetID() );
		if( player )
		{
			return player->m_Conn;
		}
	}

	return NULL;
}

void MyLogTrickObject( int skater_id, int score, uint32 num_pending_tricks, uint32* p_pending_tricks, bool propagate )
{
	Mdl::Skate * skate_mod = Mdl::Skate::Instance();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	GameNet::MsgScoreLogTrickObject msg;
	GameNet::MsgObsScoreLogTrickObject obs_msg;
	Net::MsgDesc msg_desc;
	Net::Server* server;
	Net::Conn* conn;
	GameNet::PlayerInfo* player;
	Lst::Search< GameNet::PlayerInfo > sh;

	if( propagate )
	{
		server = gamenet_man->GetServer();
		Dbg_Assert( server );

		conn = MyGetAssociatedNetworkConnection( skater_id );

		// keep track of whom we need to send the steal message to
		int i;
		char previous_owner_flags[GameNet::vMAX_PLAYERS];
		for ( i = 0; i < GameNet::vMAX_PLAYERS; i++ )
		{
			previous_owner_flags[i] = 0;
		}

		num_pending_tricks = skate_mod->GetTrickObjectManager()->RequestLogTrick( num_pending_tricks, p_pending_tricks, previous_owner_flags, skater_id, score );

		if ( !num_pending_tricks )
		{
			return;
		}

		for ( i = 0; i < GameNet::vMAX_PLAYERS; i++ )
		{
			if ( previous_owner_flags[i] )
			{
				// GJ:  This only sends the stolen message to the two parties involved

				GameNet::PlayerInfo* pPlayerInfo;
				GameNet::MsgStealMessage steal_msg;
				Net::MsgDesc steal_msg_desc;

				steal_msg.m_NewOwner = skater_id;
				steal_msg.m_OldOwner = i;
				steal_msg.m_GameId = gamenet_man->GetNetworkGameId();

				pPlayerInfo = gamenet_man->GetPlayerByObjectID(skater_id);
				Dbg_Assert( pPlayerInfo );

				steal_msg_desc.m_Data = &steal_msg;
				steal_msg_desc.m_Length = sizeof( GameNet::MsgStealMessage );
				steal_msg_desc.m_Id = GameNet::MSG_ID_STEAL_MESSAGE;
				server->EnqueueMessage( pPlayerInfo->GetConnHandle(), &steal_msg_desc );

				steal_msg.m_NewOwner = skater_id;
				steal_msg.m_OldOwner = i;
				steal_msg.m_GameId = gamenet_man->GetNetworkGameId();

				pPlayerInfo = gamenet_man->GetPlayerByObjectID(i);
				//Dbg_Assert( pPlayerInfo );

				// For now, don't assert if the player doesn't exist anymore. Just don't do anything.
				// Eventually, Gary will fix this so that pieces of exiting players are reset
				if( pPlayerInfo )
				{
					server->EnqueueMessage( pPlayerInfo->GetConnHandle(), &msg_desc );
				}
			}
		}

#ifdef __USER_GARY__
		printf( "Broadcasting %d tricks\n", num_pending_tricks );
#endif

		msg.m_SubMsgId = GameNet::SCORE_MSG_ID_LOG_TRICK_OBJECT;
		msg.m_OwnerId = skater_id;
		msg.m_Score = score;
		msg.m_NumPendingTricks = num_pending_tricks;
		msg.m_GameId = gamenet_man->GetNetworkGameId();

		uint32 max_pending_trick_buffer_size = GameNet::MsgScoreLogTrickObject::vMAX_PENDING_TRICKS * sizeof(uint32);
		uint32 actual_pending_trick_buffer_size = msg.m_NumPendingTricks * sizeof(uint32);
		memcpy( msg.m_PendingTrickBuffer, p_pending_tricks, actual_pending_trick_buffer_size );

		msg_desc.m_Data = &msg;
		msg_desc.m_Length = sizeof( GameNet::MsgScoreLogTrickObject ) - max_pending_trick_buffer_size + actual_pending_trick_buffer_size;
		msg_desc.m_Id = GameNet::MSG_ID_SCORE;
		msg_desc.m_Queue = Net::QUEUE_SEQUENCED;
		msg_desc.m_GroupId = GameNet::vSEQ_GROUP_PLAYER_MSGS;
		// tell players to change their colors
		for( player = gamenet_man->FirstPlayerInfo( sh ); player; 
			 player = gamenet_man->NextPlayerInfo( sh ))
		{
			//server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
			server->StreamMessage( player->GetConnHandle(), msg_desc.m_Id, msg_desc.m_Length, 
						   msg_desc.m_Data, "TrickObj Buffer", GameNet::vSEQ_GROUP_PLAYER_MSGS );
		}

		obs_msg.m_OwnerId = skater_id;
		obs_msg.m_NumPendingTricks = num_pending_tricks;
		obs_msg.m_GameId = gamenet_man->GetNetworkGameId();
		max_pending_trick_buffer_size = GameNet::MsgScoreLogTrickObject::vMAX_PENDING_TRICKS * sizeof(uint32);
		actual_pending_trick_buffer_size = obs_msg.m_NumPendingTricks * sizeof(uint32);
		memcpy( obs_msg.m_PendingTrickBuffer, p_pending_tricks, actual_pending_trick_buffer_size );

		msg_desc.m_Data = &obs_msg;
		msg_desc.m_Length = sizeof( GameNet::MsgObsScoreLogTrickObject ) - max_pending_trick_buffer_size + actual_pending_trick_buffer_size;
		msg_desc.m_Id = GameNet::MSG_ID_OBSERVER_LOG_TRICK_OBJ;
		// tell observers to change their colors
		for( player = gamenet_man->FirstPlayerInfo( sh, true ); player; 
			 player = gamenet_man->NextPlayerInfo( sh, true ))
		{
			if( player->IsObserving())
			{
				//server->EnqueueMessage( player->GetConnHandle(), &msg_desc );
				server->StreamMessage( player->GetConnHandle(), msg_desc.m_Id, msg_desc.m_Length, 
						   msg_desc.m_Data, "TrickObj Buffer", GameNet::vSEQ_GROUP_PLAYER_MSGS );
			}
		}

		// send score updates, as something has changed
		skate_mod->SendScoreUpdates( false );

		// Let the server's client do the rest of the work
		if( conn->IsLocal())
		{
			return;
		}
	}

#ifdef __USER_GARY__
	printf( "Client is receiving %d tricks\n", num_pending_tricks );
#endif

	// modulate the color here
	for ( uint32 i = 0; i < num_pending_tricks; i++ )
	{
		skate_mod->GetTrickObjectManager()->ModulateTrickObjectColor( p_pending_tricks[i], skater_id );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void trick_off_object(CTrickCluster* pCluster, void* pData)
{
	

	Dbg_AssertPtr(pCluster);

	SAutoTrick* pAutoTrick = (SAutoTrick*)pData;

	if ( pAutoTrick->numPendingTricks >= 32 )
	{
		return;
	}

	if ( !pCluster->m_IsOwned )
	{
		pAutoTrick->pPendingTricks[pAutoTrick->numPendingTricks] = pCluster->GetNameChecksum();
		pAutoTrick->numPendingTricks++;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickObjectManager::TrickOffAllObjects( uint32 skater_id )
{
	SAutoTrick theAutoTrick;
	theAutoTrick.numPendingTricks = 0;
	
	m_TrickClusterList.HandleCallback(trick_off_object, &theAutoTrick);

	if ( theAutoTrick.numPendingTricks > 0 )
	{
		MyLogTrickObject( skater_id, 1000, theAutoTrick.numPendingTricks, theAutoTrick.pPendingTricks, true );
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickObjectManager::ModulateTrickObjectColor( uint32 name_checksum, int skater_id )
{
	

	CTrickCluster* pCluster = get_aliased_cluster( name_checksum );
	
	// change only objects marked as trick objects
	if ( !pCluster )
	{
		// not a trick object
		return false;
	}

	// convert skater_id ->seqIndex, as it's 1-based,
	// or -1 to reset
	int seqIndex = -1;
	if ( skater_id != -1 )
	{
		seqIndex = skater_id + 1;
	}
	
	Dbg_MsgAssert( seqIndex == -1 || ( seqIndex >= 1 && seqIndex <= 8 ) ,( "Out of range seq index %d", seqIndex ));

	pCluster->ModulateTrickObjectColor( seqIndex );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTrickCluster* CTrickObjectManager::get_aliased_cluster( uint32 alias_checksum )
{
#if 0
	for ( uint32 i = 0; i < m_TrickAliasCount; i++ )
	{
		if ( m_TrickAliasList[i].m_AliasChecksum == alias_checksum )
		{
			return m_TrickAliasList[i].mp_TrickCluster;
		}
	}
	return NULL;
#endif
	return m_TrickAliasList.GetItem(alias_checksum, false);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CTrickCluster* CTrickObjectManager::find_trick_cluster( uint32 cluster_checksum )
{
	

#if 0
	uint32 count = m_TrickClusterList.CountItems();

	// find the appropriate trick object
	for ( uint32 i = 0; i < count; i++ )
	{
		Lst::Node< CTrickCluster >* pNode = m_TrickClusterList.GetItem( i );
		Dbg_Assert( pNode );
		CTrickCluster* pCluster = pNode->GetData();
		Dbg_Assert( pCluster );

		if ( pCluster->m_NameChecksum == cluster_checksum )
		{
			return pCluster;
		}
	}
	return NULL;
#endif
	return m_TrickClusterList.GetItem( cluster_checksum, false );	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickObjectManager::clear_trick_clusters( int seqIndex )
{

	m_TrickClusterList.IterateStart();
	CTrickCluster *p_cluster;
	while ((p_cluster = m_TrickClusterList.IterateNext()))
	{
		if (!p_cluster->ClearCluster(seqIndex))
		{
			return false;
		}
	}

	return true;
}

// this is useful if you want to examine the functionality
// of a single trick object without sorting through hundreds of
// other trick objects...  it represents one single piece in
// the foundry
//#define __TESTSINGLETRICKOBJECT__

#ifdef __TESTSINGLETRICKOBJECT__
	const char* p_single_cluster_name = "ParkingLotBulldozer";
	const char* p_single_object_name = "Parking_Lot_Bulldozer0";
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickObjectManager::AddTrickCluster( uint32 name_checksum )
{
	


#ifdef __TESTSINGLETRICKOBJECT__
	if ( name_checksum != Script::GenerateCRC(p_single_cluster_name) )
		return false;
#endif

#ifdef __USER_GARY__
//	printf( "+++  Adding trick object %s (%x)\n", Script::FindChecksumName(name_checksum), name_checksum );
#endif

	if ( find_trick_cluster( name_checksum ) )
	{
		return false;
	}
	
	// If playing a created park, clear any created rail colors for this cluster.
	Mdl::Skate * p_skate_mod =  Mdl::Skate::Instance();
	if (p_skate_mod->m_cur_level == CRCD(0xb664035d,"load_sk5ed_gameplay"))
	{
		Obj::GetRailEditor()->ClearRailColor(name_checksum);
	}
	
	// add the cluster since it doesn't already exist
	CTrickCluster* pCluster = new CTrickCluster( name_checksum );
	Dbg_Assert( pCluster );
//	m_TrickClusterList.AddToTail( pCluster );
	m_TrickClusterList.PutItem( name_checksum, pCluster );
	m_NumTrickClusters++;
	
#if 0	
	Ed::ParkEditor* park_ed_mod = Ed::ParkEditor::Instance();
	if (!park_ed_mod->IsInitialized())
	{
		Dbg_Assert( m_NumTrickClusters <= 256 );
	}
#endif
	
	// alias the cluster to itself
	AddTrickAlias( name_checksum, name_checksum );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickObjectManager::AddTrickObjectToCluster( uint32 name_checksum, uint32 cluster_checksum )
{
	
	
#ifdef __TESTSINGLETRICKOBJECT__
	if ( name_checksum != Script::GenerateCRC(p_single_object_name) )
		return false;
#endif

	CTrickCluster* pCluster = find_trick_cluster( cluster_checksum );
	Dbg_MsgAssert( pCluster,( "cluster %s not found", Script::FindChecksumName( cluster_checksum ) ));

	bool success = pCluster->AddTrickObject( name_checksum );

	// only if not clustering to self
	if ( success && ( name_checksum != cluster_checksum ) )
	{
		// bind the environment object to the cluster
		AddTrickAlias( name_checksum, cluster_checksum );
	}
	
	return success;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickObjectManager::AddTrickAlias( uint32 alias_checksum, uint32 cluster_checksum )
{
	
	
#ifdef __TESTSINGLETRICKOBJECT__
	if ( cluster_checksum != Script::GenerateCRC(p_single_cluster_name) )
		return false;
#endif
	
	CTrickCluster* pCluster = get_aliased_cluster( alias_checksum );

	if( pCluster )
	{
		Dbg_MsgAssert( 0,( "%s has already been aliased to %s cluster", Script::FindChecksumName(alias_checksum), Script::FindChecksumName(pCluster->m_NameChecksum) ));
	}

	pCluster = find_trick_cluster( cluster_checksum );

	if( pCluster == NULL )
	{
		Dbg_MsgAssert( 0,( "cluster %s not found", Script::FindChecksumName(cluster_checksum) ));
	}

	m_TrickAliasList.PutItem(alias_checksum, pCluster);
	
#if 0
	Dbg_MsgAssert( m_TrickAliasCount < vMAX_ALIASES,( "Too many aliases.  Couldn't alias %s to cluster %s", Script::FindChecksumName(alias_checksum), Script::FindChecksumName(cluster_checksum) ));
	m_TrickAliasList[m_TrickAliasCount].m_AliasChecksum = alias_checksum;
	pCluster = find_trick_cluster( cluster_checksum );
	m_TrickAliasList[m_TrickAliasCount].mp_TrickCluster = pCluster;
	Dbg_MsgAssert( pCluster,( "cluster %s not found", Script::FindChecksumName(cluster_checksum) ));
	m_TrickAliasCount++;
#endif

//	printf("Adding trick alias %s\n", Script::FindChecksumName(alias_checksum));
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void delete_trick_cluster(CTrickCluster* pCluster, void* pData)
{
	

	Dbg_AssertPtr(pCluster);

	delete pCluster;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickObjectManager::DeleteAllTrickObjects( void )
{
	

#if 0
	uint32 count = m_TrickClusterList.CountItems();
	for ( uint32 i = 0; i < count; i++ )
	{
		Lst::Node< CTrickCluster >* pNode = m_TrickClusterList.GetItem( 0 );
		Dbg_Assert( pNode );
		CTrickCluster* pCluster = pNode->GetData();
		Dbg_Assert( pCluster );
		delete pCluster;
	}

	// now remove all the nodes from the list
	m_TrickClusterList.RemoveAllNodes();
#endif

	m_TrickClusterList.HandleCallback(delete_trick_cluster, NULL);
	m_TrickClusterList.FlushAllItems();
	m_NumTrickClusters = 0;
	
//	m_TrickAliasCount = 0;
	m_TrickAliasList.FlushAllItems();
	
	return true;
}

struct STrickObjectScore
{
	int		running_score;
	uint32	skater_id;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void count_score(CTrickCluster* pCluster, void* pData)
{
	

	Dbg_AssertPtr(pCluster);
	Dbg_AssertPtr(pData);
	
	STrickObjectScore* pScore = (STrickObjectScore*)pData;
	pScore->running_score += pCluster->GetScore( pScore->skater_id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int CTrickObjectManager::GetScore( uint32 skater_id )
{
	STrickObjectScore theScore;
	theScore.running_score = 0;
	theScore.skater_id = skater_id;

	m_TrickClusterList.HandleCallback(count_score, (void*)&theScore);

	return theScore.running_score;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

struct SSearchInfo
{
	uint32 checksum;
	uint8 index;
	bool found;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void find_compressed_index(CTrickCluster* pCluster, void* pData)
{
	

	Dbg_AssertPtr(pCluster);
	Dbg_AssertPtr(pData);

	SSearchInfo* pInfo = (SSearchInfo*)pData;
	
	if ( pInfo->found )
	{
		// already found...
		return;
	}
	
	if ( pInfo->checksum == pCluster->GetNameChecksum() )
	{
		pInfo->found = true;
		return;
	}

	// make sure it's less than 256
	Dbg_MsgAssert( pInfo->index < 256, ( "Too many trick aliases" ) );
	
	pInfo->index++;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void find_uncompressed_checksum(CTrickCluster* pCluster, void* pData)
{
	

	Dbg_AssertPtr(pCluster);
	Dbg_AssertPtr(pData);

	SSearchInfo* pInfo = (SSearchInfo*)pData;

	if ( pInfo->found )
	{
		// already found...
		return;
	}

	if ( pInfo->index == 0 )
	{
		pInfo->found = true;
		pInfo->checksum = pCluster->GetNameChecksum();
		return;
	}

	pInfo->index--;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint8 CTrickObjectManager::GetCompressedTrickObjectIndex( uint32 name_checksum )
{
	
	
	SSearchInfo theInfo;
	theInfo.index = 0;
	theInfo.checksum = name_checksum;
	theInfo.found = false;
	
	m_TrickClusterList.HandleCallback( find_compressed_index, (void*)&theInfo );

	Dbg_Assert( theInfo.found );

	return theInfo.index;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CTrickObjectManager::GetUncompressedTrickObjectChecksum( uint8 compressed_index )
{
	
	
	SSearchInfo theInfo;
	theInfo.index = compressed_index;
	theInfo.checksum = 0;
	theInfo.found = false;

	m_TrickClusterList.HandleCallback( find_uncompressed_checksum, (void*)&theInfo );

	Dbg_Assert( theInfo.found );
	
	return theInfo.checksum;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

struct SInitGraffitiStateInfo
{
	uint32 skater_id;
	int num_trick_objects;
	GameNet::MsgInitGraffitiState* pMsg;
	Obj::CTrickObjectManager* pManager;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void init_graffiti_state(CTrickCluster* pCluster, void* pData)
{
	
	
	SInitGraffitiStateInfo* pInfo = (SInitGraffitiStateInfo*)pData;

	if ( pCluster->m_IsOwned && pCluster->m_OwnerId == pInfo->skater_id )
	{
		// TODO:  check whether it's already in the list?
		
		Dbg_MsgAssert( pInfo->num_trick_objects < GameNet::vMAX_TRICK_OBJECTS_IN_LEVEL, ( "Too many trick objects in level" ) );
		
		Obj::CTrickObjectManager* pManager = pInfo->pManager;
		pInfo->pMsg->m_TrickObjectStream[pInfo->num_trick_objects] = pManager->GetCompressedTrickObjectIndex( pCluster->GetNameChecksum() );
		pInfo->num_trick_objects++;
		pInfo->pMsg->m_NumTrickObjects[pInfo->skater_id]++;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickObjectManager::SetObserverGraffitiState( Script::CScriptStructure* pScriptStructure )
{
	mp_ObserverState->Clear();
	if (pScriptStructure)
	{
		*mp_ObserverState+=*pScriptStructure;
	}	

#ifdef __NOPT_ASSERT__
	Script::PrintContents(mp_ObserverState);
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickObjectManager::ApplyObserverGraffitiState( void )
{
	for( int i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
	{
		Script::CArray* pArray;

		char arrayName[32];
		sprintf( arrayName, "Skater%d", i );

		if ( mp_ObserverState->GetArray( arrayName, &pArray ) )
		{
			for ( int j = 0; j < (int)pArray->GetSize(); j++ )
			{			
				uint32 checksum = pArray->GetNameChecksum( j );
#ifdef __NOPT_ASSERT__				
				printf( "Modulating array %s\n", Script::FindChecksumName(checksum) );
#endif
				ModulateTrickObjectColor( checksum, i );
			}
		}
	}

	// now that we've done it, we can clear the observer state.
	mp_ObserverState->Clear();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CTrickObjectManager::SetInitGraffitiStateMessage( void* pData )
{
	

	SInitGraffitiStateInfo theInfo;
	theInfo.pMsg = (GameNet::MsgInitGraffitiState*)pData;
	theInfo.pManager = this;
	theInfo.num_trick_objects = 0;

	for ( uint32 i = 0; i < Mdl::Skate::vMAX_SKATERS; i++ )
	{
		theInfo.pMsg->m_NumTrickObjects[i] = 0;
		theInfo.skater_id = i;
		m_TrickClusterList.HandleCallback( init_graffiti_state, (void*)&theInfo );
        
		printf( "Building graffiti state message %d\n", theInfo.pMsg->m_NumTrickObjects[i] );
	}
	
	return ( sizeof(uint8) * Mdl::Skate::vMAX_SKATERS ) + ( sizeof(uint8) * theInfo.num_trick_objects );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CTrickObjectManager::ResetAllTrickObjects( void )
{
	

	// TODO:
	printf("Resetting all trick objects\n");
	return false;

#if 0
	uint32 count = m_TrickClusterList.CountItems();

	// call the reset function on each trick object in the scene
	for ( uint32 i = 0; i < count; i++ )
	{
		Lst::Node< CTrickCluster >* pNode = m_TrickClusterList.GetItem( i );
		Dbg_Assert( pNode );
		CTrickCluster* pCluster = pNode->GetData();
		Dbg_Assert( pCluster );
		pCluster->Reset();
	}
#endif
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CTrickObjectManager::PrintContents( void )
{
	

	// Remove this line if you need to see the contents
	return;

	printf("**********************\n");
	printf("* TRICK CLUSTER LIST:\n");
	printf("**********************\n");

#if 0
	uint32 i;

	// call the reset function on each trick object in the scene
	for ( i = 0; i < m_TrickClusterList.CountItems(); i++ )
	{
		Lst::Node< CTrickCluster >* pNode = m_TrickClusterList.GetItem( i );
		Dbg_Assert( pNode );
		CTrickCluster* pCluster = pNode->GetData();
		Dbg_Assert( pCluster );
		printf( "Trick cluster %d of %d: %s\n", i, m_TrickClusterList.CountItems(), Script::FindChecksumName( pCluster->m_NameChecksum ) );
	}
#endif
	
#if 0
	printf("**********************\n");
	printf("* TRICK ALIAS LIST:\n");
	printf("**********************\n");

	for ( i = 0; i < m_TrickAliasCount; i++ )
	{
//		printf( "%d: %s is aliased to %s\n", i, Script::FindChecksumName(m_TrickAliasList[i].m_AliasChecksum), Script::FindChecksumName(m_TrickAliasList[i].mp_TrickCluster->m_NameChecksum) );
	}
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CPendingTricks::CPendingTricks( void )
{
	m_NumTrickItems = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPendingTricks::TrickOffObject( uint32 checksum )
{
	
	
	uint32 num_trick_items = m_NumTrickItems;
	if ( num_trick_items > vMAX_PENDING_TRICKS )
		num_trick_items = vMAX_PENDING_TRICKS;

	// not a valid trick item
	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	CTrickObjectManager* pTrickObjectManager = skate_mod->GetTrickObjectManager();
	Dbg_Assert( pTrickObjectManager );
	checksum = pTrickObjectManager->TrickObjectExists( checksum );
	if ( !checksum )
	{
		return false;
	}
	
	for ( uint32 i = 0; i < num_trick_items; i++ )
	{
		if ( m_Checksum[i] == checksum )
			return false;
	}

//	printf( "Adding object %s to pending tricks:\n", Script::FindChecksumName(checksum) );
	
	// circular buffer
	m_Checksum[(m_NumTrickItems++)%vMAX_PENDING_TRICKS] = checksum;
	
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

uint32 CPendingTricks::WriteToBuffer( uint32* p_buffer, uint32 max_size )
{
	

	// TODO:  Account for the circular buffer so that the buffer
	// size can be bigger than the pending trick size
	Dbg_Assert( ( vMAX_PENDING_TRICKS * sizeof(uint32) ) <= max_size );
	
	uint32 num_trick_items = m_NumTrickItems;
	if ( num_trick_items > vMAX_PENDING_TRICKS )
		num_trick_items = vMAX_PENDING_TRICKS;

	Mdl::Skate* skate_mod = Mdl::Skate::Instance();
	CTrickObjectManager* pTrickObjectManager = skate_mod->GetTrickObjectManager();
	Dbg_Assert( pTrickObjectManager );

	uint32 size = 0;
	for ( uint32 i = 0; i < num_trick_items; i++ )
	{
#ifdef __NOPT_ASSERT__
		printf( "Looking for %s\n", Script::FindChecksumName( m_Checksum[i] ) );
#endif		
		if ( pTrickObjectManager->TrickObjectExists( m_Checksum[i] ) )
		{
			*p_buffer = m_Checksum[i];
			p_buffer++ ;

			// increment the size
			size += sizeof(uint32);
		}
		
		if ( size >= max_size )
			break;
	}
	return size;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool CPendingTricks::FlushTricks( void )
{
	// player has bailed
	m_NumTrickItems = 0;

	return true;
}

} // namespace Obj


