///********************************************************************************
// *																				*
// *	Module:																		*
// *				Prim															*
// *	Description:																*
// *				Provides functionality for drawing various types of single		*
// *				primitives to the display.										*
// *	Written by:																	*
// *				Paul Robinson													*
// *	Copyright:																	*
// *				2001 Neversoft Entertainment - All rights reserved.				*
// *																				*
// ********************************************************************************/
//
///********************************************************************************
// * Includes.																	*
// ********************************************************************************/
//#include <core/defines.h>
//#include <core/crc.h>
//#include <gel/scripting/script.h>
//#include <math.h>
//#include "p_texman.h"
//#include "p_hashid.h"
//#include "p_assert.h"
//#include "p_file.h"
//
///********************************************************************************
// * Defines.																		*
// ********************************************************************************/
//
//#define DEFAULT_TEXMAN_HASH_SIZE 2048
//
///********************************************************************************
// * Structures.																	*
// ********************************************************************************/
//
///********************************************************************************
// * Local variables.																*
// ********************************************************************************/
////static NsTexture *htab[2048];
////static NsTexture *ltab[2048];
//
///********************************************************************************
// * Forward references.															*
// ********************************************************************************/
//
///********************************************************************************
// * Externs.																		*
// ********************************************************************************/
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				NsTextureMan															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Instances a texture manager to the default hash table size of	*
// *				2048 entries.													*
// *																				*
// ********************************************************************************/
//NsTextureMan::NsTextureMan()
//{
//	NsTextureMan ( DEFAULT_TEXMAN_HASH_SIZE );
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				NsTextureMan															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Instances a texture manager to a user-specified size.			*
// *																				*
// ********************************************************************************/
//NsTextureMan::NsTextureMan ( int hashTableSize )
//{
////	m_pHashTable = (NsTexture **)new unsigned char[ sizeof ( NsTexture ) * hashTableSize];
////	assert ( m_pHashTable );
////	m_tableSize = hashTableSize;
////
////	reset();
//
//	assertf( false, ( "No longer used...\n" ) );
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				~NsTextureMan													*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Destroys a texture manager.										*
// *																				*
// ********************************************************************************/
//NsTextureMan::~NsTextureMan()
//{
//	int lp;
//
//	// Destroy all textures.
//	for ( lp = 0; lp < (int)m_tableSize; lp++ ) {
//		if ( m_pHashTable[lp] ) delete m_pHashTable[lp];
//	}
//	delete (unsigned int *)m_pHashTable;
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				NsTextureMan													*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Instances a texture manager big enough to hold the specified	*
// *				file, loads and adds it.										*
// *																				*
// ********************************************************************************/
//NsTextureMan::NsTextureMan ( const char * pFilename )
//{
//	NsFile			file;
//	unsigned int  * p32;
//	unsigned int	hashTableSize;
//
//	// Load the texture binary.
//	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
//	p32 = (unsigned int *)file.load( pFilename );
//	Mem::Manager::sHandle().PopContext();
//
//	// Calculate size of hash table.
//	hashTableSize = p32[0] * 2;
//
//	// Create space for hash table.
//	m_pHashTable = (NsTexture **)new unsigned int[hashTableSize];
//	assert ( m_pHashTable );
//	m_tableSize = hashTableSize;
//	reset();
//
//	// Add texture to hash table.
//	addList ( p32 );
//
//	// Delete the original.
//	delete p32;
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				merge															*
// *	Inputs:																		*
// *				<none>															*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Merges the specified texture manager into the current one.		*
// *				Basically, it creates a new texture has array big enough for	*
// *				both lists and re-adds everything.								*
// *																				*
// ********************************************************************************/
//void NsTextureMan::merge ( NsTextureMan& source )
//{
////	NsTexture	 ** pOldTable;
////	unsigned int	oldTableSize;
////	unsigned int	newTableSize;
//	unsigned int	lp;
//
////	// Calculate size of new hash table.
////	newTableSize = m_tableSize + source.m_tableSize;
////
////	// Save old table information.
////	oldTableSize = m_tableSize;
////	pOldTable = m_pHashTable;
////
////	// Create space for new hash table.
////	m_pHashTable = (NsTexture **)new unsigned int[newTableSize];
////	assert ( m_pHashTable );
////	m_tableSize = newTableSize;
////	reset();
////
////	// Add each texture in turn from the old table.
////	for ( lp = 0; lp < oldTableSize; lp++ ) {
////		if ( pOldTable[lp] ) {
////			// This should turn into a 'move' command.
////			{
////				unsigned int	hashValue;
////				unsigned int	hashSearch;
////				
////				hashValue = Script::GenerateCRC ( pOldTable[lp]->m_name );
////				hashValue %= m_tableSize;
////				hashSearch = hashValue;
////				
////				do {
////					// See if this hash slot is empty.
////					if ( !m_pHashTable[hashSearch] ) {
////						// Copy the pointer to the new hash table.
////						m_pHashTable[hashSearch] = pOldTable[lp];
////						break;
////					}
////					// Next hash entry.
////					hashSearch = ( hashSearch + 1 ) % m_tableSize;
////				} while ( hashValue != hashSearch );
////			}
////		}
////	}
////
//	// Add each texture in turn from the new table (cloned as the source table will
//	// almost certainly be deleted).
//	for ( lp = 0; lp < source.m_tableSize; lp++ ) {
//		if ( source.m_pHashTable[lp] ) {
//			add( source.m_pHashTable[lp] );
//			source.m_pHashTable[lp] = NULL;
//		}
//	}
//
////	// Delete old hash table.
////	delete pOldTable;
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				add																*
// *	Inputs:																		*
// *				pTexture	The texture to add.									*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Adds the specified texture to the texture manager.				*
// *																				*
// ********************************************************************************/
//NsTexture * NsTextureMan::_add( NsTexture * pTexture, unsigned int copy )
//{
//	unsigned int	hashValue;
//	unsigned int	hashSearch;
//	char		  * p8;
//	NsTexture	  * pTex;
//
//	pTex = pTexture;
//
//	assert ( m_allocatedCount < m_tableSize );
//	
//	// Set the texture name checksum.
//	pTexture->m_id = Script::GenerateCRC ( pTexture->m_name );
//
//	hashValue = pTexture->m_id;
//	hashValue %= m_tableSize;
//	hashSearch = hashValue;
//	
//	do {
//		// See if this hash slot is empty.
//		if ( !m_pHashTable[hashSearch] ) {
//			// Set palette if required & point past end.
//			p8 = (char *)&pTexture[1];
//			switch ( pTexture->m_depth ) {
//				case 4:
//					p8 += ( pTexture->m_width * pTexture->m_height ) / 2;
//					pTex = (NsTexture *)&p8[16*2];
//					break;
//				case 8:
//					p8 += pTexture->m_width * pTexture->m_height;
//					pTex = (NsTexture *)&p8[256*2];
//					break;
//				case 16:
//					p8 += pTexture->m_width * pTexture->m_height * 2;
//					pTex = (NsTexture *)p8;
//					break;
//				case 32:
//					p8 += pTexture->m_width * pTexture->m_height * 4;
//					pTex = (NsTexture *)p8;
//					break;
//				default:
//					break;
//			}
//			NsTexture	  * newTex;
//			if ( copy ) {
//				// Copy the texture header.
//				newTex = new NsTexture;
//				memcpy ( newTex, pTexture, sizeof( NsTexture ) );
//				// Copy the texture data.
//				int size = (((int)pTex)-((int)&pTexture[1]));
//				newTex->m_pImage = new unsigned char[size];
//				memcpy ( newTex->m_pImage, &pTexture[1], size );
//			} else {
//				newTex = pTexture;
//				if ( !newTex->m_pImage ) newTex->m_pImage = &newTex[1];
//			}
//
//			// Set texture parameters.
//			newTex->m_uploaded = 0;
//
//			// Point to the palette.
//			if ( !newTex->m_pPalette ) {
//				p8 = (char *)newTex->m_pImage;
//				switch ( newTex->m_depth ) {
//					case 4:
//						p8 += ( newTex->m_width * newTex->m_height ) / 2;
//						newTex->m_pPalette = p8;
//						break;
//					case 8:
//						p8 += newTex->m_width * newTex->m_height;
//						newTex->m_pPalette = p8;
//						break;
//					default:
//						break;
//				}
//			}
//			// Found an empty slot, point it up.
//			m_pHashTable[hashSearch] = newTex;
//
//			// We're done.
//			m_allocatedCount++;
//			break;
//		}
//		// Next hash entry.
//		hashSearch = ( hashSearch + 1 ) % m_tableSize;
//	} while ( hashValue != hashSearch );
//
//	return pTex;
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				add																*
// *	Inputs:																		*
// *				pTexture	The texture to add.									*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Adds the specified texture to the texture manager.				*
// *																				*
// ********************************************************************************/
//NsTexture * NsTextureMan::add( NsTexture * pTexture )
//{
//	return _add( pTexture, 0 );
//}
//
//NsTexture * NsTextureMan::addClone( NsTexture * pTexture )
//{
//	return _add( pTexture, 1 );
//}
//
//void NsTextureMan::remove ( const char * pName )
//{
//	unsigned int	hashID;
//	unsigned int	hashValue;
//	unsigned int	hashSearch;
//	
//	hashID = Script::GenerateCRC ( pName );
//	hashValue = hashID % m_tableSize;
//	hashSearch = hashValue;
//	
//	if ( !m_tableSize || !m_pHashTable ) return;
//
//	do {
//		// See if this hash slot matches.
//		if ( m_pHashTable[hashSearch] ) {
//			if ( m_pHashTable[hashSearch]->m_id == hashID ) {
//				// Found the matching item.
//				delete m_pHashTable[hashSearch];
//				m_pHashTable[hashSearch] = NULL;
//				break;
//			}
//		}
//		// Next hash entry.
//		hashSearch = ( hashSearch + 1 ) % m_tableSize;
//	} while ( hashValue != hashSearch );
//}
//
//void NsTextureMan::replace ( const char * pName, NsTexture *pReplacement )
//{
//	unsigned int	hashID;
//	unsigned int	hashValue;
//	unsigned int	hashSearch;
//	
//	hashID = Script::GenerateCRC ( pName );
//	hashValue = hashID % m_tableSize;
//	hashSearch = hashValue;
//
//	if ( !m_tableSize || !m_pHashTable ) return;
//
//	do {
//		// See if this hash slot matches.
//		if ( m_pHashTable[hashSearch] ) {
//			if ( m_pHashTable[hashSearch]->m_id == hashID ) {
//				// Found the matching item.
//
//				// Delete the old image.
//				delete (unsigned char *)m_pHashTable[hashSearch]->m_pImage;
//
//				// Copy the new texture details over (only the dimensions).
//				m_pHashTable[hashSearch]->m_width	= pReplacement->m_width;
//				m_pHashTable[hashSearch]->m_height	= pReplacement->m_height;
//				m_pHashTable[hashSearch]->m_depth	= pReplacement->m_depth;
//
//				// Copy new image data & assign.
//				int size = ( pReplacement->m_width * pReplacement->m_height * pReplacement->m_depth ) / 8;
//				int palsize = ( pReplacement->m_depth < 16 ) ? ( 2 << pReplacement->m_depth ) : 0;
//				m_pHashTable[hashSearch]->m_pImage = new unsigned char[(size+palsize)];
//				memcpy( m_pHashTable[hashSearch]->m_pImage, &pReplacement[1], (size+palsize) );
//
//				// Set palette pointer.
//				unsigned char * p8 = (unsigned char *)m_pHashTable[hashSearch]->m_pImage;
//				m_pHashTable[hashSearch]->m_pPalette = &p8[size];
//				break;
//			}
//		}
//		// Next hash entry.
//		hashSearch = ( hashSearch + 1 ) % m_tableSize;
//	} while ( hashValue != hashSearch );
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				retrieve														*
// *	Inputs:																		*
// *				pName	The name of the texture to retrieve.					*
// *	Output:																		*
// *				A pointer to the requested texture. NULL if not found.			*
// *	Description:																*
// *				Retrieves the specified texture from the texture manager.		*
// *																				*
// ********************************************************************************/
//NsTexture * NsTextureMan::retrieve ( const char * pName )
//{
//	unsigned int	hashID;
//	unsigned int	hashValue;
//	unsigned int	hashSearch;
//	NsTexture	  * pHeader;
//	
//	hashID = Script::GenerateCRC ( pName );
//	hashValue = hashID % m_tableSize;
//	hashSearch = hashValue;
//	pHeader = NULL;
//	
//	if ( !m_tableSize || !m_pHashTable ) return NULL;
//
//	do {
//		// See if this hash slot matches.
//		if ( m_pHashTable[hashSearch] ) {
//			if ( m_pHashTable[hashSearch]->m_id == hashID ) {
//				// Found the matching item.
//				pHeader = m_pHashTable[hashSearch];
//				break;
//			}
//		}
//		// Next hash entry.
//		hashSearch = ( hashSearch + 1 ) % m_tableSize;
//	} while ( hashValue != hashSearch );
//	
//	return pHeader;
//}
//
///********************************************************************************
// *																				*
// *	Method:																		*
// *				addList															*
// *	Inputs:																		*
// *				pTexList	Pointer to a list of textures to add to the texture	*
// *							manager.											*
// *	Output:																		*
// *				<none>															*
// *	Description:																*
// *				Adds the specified list of textures to the texture manager.		*
// *				The data passed to this function corresponds to the binary data	*
// *				output of CRW.													*
// *																				*
// ********************************************************************************/
//void NsTextureMan::addList ( void * pTexList )
//{
//	unsigned int	numTex;
//	unsigned int	tex;
//	NsTexture	  *	pTex;
////	char		  * p8;
//	unsigned int  * p32;
//	
//	// Go through texture file, adding all textures to texture manager.
//	p32 = (unsigned int *)pTexList;
//	numTex = p32[0];
//	pTex = (NsTexture *)&p32[8];
//	for ( tex = 0; tex < numTex; tex++ ) {
//		assert ( pTex->m_id == *((unsigned int *)"GCTX") );
//		pTex = addClone ( pTex );
//	}
//}
//
//void NsTextureMan::reset ( void )
//{
//	int lp;
//
//	// NULL the hashtable out.
//	for ( lp = 0; lp < (int)m_tableSize; lp++ ) {
//		m_pHashTable[lp] = NULL;
//	}
//	// Set number of textures to 0.
//	m_allocatedCount = 0;
//}

