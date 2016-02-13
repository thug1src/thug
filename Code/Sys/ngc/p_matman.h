//#ifndef _MATMAN_H_
//#define _MATMAN_H_
//
//#include "p_hw.h"
//#include "p_material.h"
//#include "p_texman.h"
//#include "p_collision.h"
//#include "p_dl.h"
//#include "p_triangle.h"
//
//class NsMaterialMan
//{
//	int				m_refCount;
//public:
//	NsMaterial	  * m_pMaterialList;
//	NsMaterial	 ** m_pSortedMaterialList;
//	int				m_materialListSize;
//	int				m_numTotal;
//	int				m_numCulled;
//
//	friend class NsModel;
//
//					NsMaterialMan			();
//					NsMaterialMan			( unsigned int numEntries );
//					~NsMaterialMan			();
//
//	NsMaterial	  * retrieve				( unsigned int number );
//
//	void			draw					( void );
//
//	unsigned int	cull					( NsMatrix * m );
//
//	void			reset					( void );
//
//	void			calculateBoundingBox	( NsCull_Item * pCull );
//
//	int				findCollision			( NsLine * pLine, NsDL::Collision_LineCallback pCb, void * pData, void* pWorld );
//
//	void			addReference			( void ) { m_refCount++; }
//	void			removeReference			( void ) { m_refCount--; }
//	int				totalReferences			( void ) { return m_refCount; }
//};
//
//#endif		// _MATMAN_H_









