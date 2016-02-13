#ifndef _REFTYPES_H_
#define _REFTYPES_H_

#include <dolphin.h>

////////////////////////////////////////////////////////////

class NsRefDoubleData
{
	int				m_refCount;
public:
	unsigned int  * m_pDoubleData;

					NsRefDoubleData		( unsigned int * p ) { m_refCount = 0; m_pDoubleData = p; }
					~NsRefDoubleData	() { if ( m_pDoubleData ) delete m_pDoubleData; }

	void			addReference		( void ) { m_refCount++; }
	void			removeReference		( void ) { m_refCount--; }
	int				totalReferences		( void ) { return m_refCount; }
};

////////////////////////////////////////////////////////////

class NsRefAccData
{
	int				m_refCount;
public:
	unsigned int  * m_pAccData;

					NsRefAccData	( unsigned int * p ) { m_refCount = 0; m_pAccData = p; }
					~NsRefAccData	() { if ( m_pAccData ) delete m_pAccData; }

	void			addReference	( void ) { m_refCount++; }
	void			removeReference	( void ) { m_refCount--; }
	int				totalReferences	( void ) { return m_refCount; }
};

////////////////////////////////////////////////////////////

class NsRefTransformedVertices
{
	int						m_refCount;
public:
	NsAnim_PosNormPair	  * m_pTransformedVertices;

							NsRefTransformedVertices	( NsAnim_PosNormPair * p ) { m_refCount = 0; m_pTransformedVertices = p; }
							~NsRefTransformedVertices	() { if ( m_pTransformedVertices ) delete m_pTransformedVertices; }

	void					addReference				( void ) { m_refCount++; }
	void					removeReference				( void ) { m_refCount--; }
	int						totalReferences				( void ) { return m_refCount; }
};

////////////////////////////////////////////////////////////

class NsRefSkinBox
{
	int				m_refCount;
public:
	NsBBox		  * m_pSkinBox;

					NsRefSkinBox	( NsBBox * p ) { m_refCount = 0; m_pSkinBox = p; }
					~NsRefSkinBox	() { if ( m_pSkinBox ) delete m_pSkinBox; }

	void			addReference	( void ) { m_refCount++; }
	void			removeReference	( void ) { m_refCount--; }
	int				totalReferences	( void ) { return m_refCount; }
};

////////////////////////////////////////////////////////////

class NsRefCAS16
{
	int					m_refCount;
public:
	unsigned short	  * m_pCAS16;

						NsRefCAS16		( unsigned short * p ) { m_refCount = 0; m_pCAS16 = p; }
						~NsRefCAS16		() { if ( m_pCAS16 ) delete m_pCAS16; }

	void				addReference	( void ) { m_refCount++; }
	void				removeReference	( void ) { m_refCount--; }
	int					totalReferences	( void ) { return m_refCount; }
};

////////////////////////////////////////////////////////////

class NsRefFlipPairs
{
	int				m_refCount;
public:
	unsigned int  * m_pFlipPairs;

					NsRefFlipPairs	( unsigned int * p ) { m_refCount = 0; m_pFlipPairs = p; }
					~NsRefFlipPairs		() { if ( m_pFlipPairs ) delete m_pFlipPairs; }

	void			addReference	( void ) { m_refCount++; }
	void			removeReference	( void ) { m_refCount--; }
	int				totalReferences	( void ) { return m_refCount; }
};

////////////////////////////////////////////////////////////

class NsRefCAS32
{
	int				m_refCount;
public:
	unsigned int  * m_pCAS32;

					NsRefCAS32		( unsigned int * p ) { m_refCount = 0; m_pCAS32 = p; }
					~NsRefCAS32		() { if ( m_pCAS32 ) delete m_pCAS32; }

	void			addReference	( void ) { m_refCount++; }
	void			removeReference	( void ) { m_refCount--; }
	int				totalReferences	( void ) { return m_refCount; }
};

////////////////////////////////////////////////////////////

class NsRefPartChecksums
{
	int				m_refCount;
public:
	unsigned int  * m_pPartChecksums;

					NsRefPartChecksums	( unsigned int * p ) { m_refCount = 0; m_pPartChecksums = p; }
					~NsRefPartChecksums	() { if ( m_pPartChecksums ) delete m_pPartChecksums; }

	void			addReference		( void ) { m_refCount++; }
	void			removeReference		( void ) { m_refCount--; }
	int				totalReferences		( void ) { return m_refCount; }
};

////////////////////////////////////////////////////////////

class NsRefBoneData
{
	int					m_refCount;
public:
	NsAnim_BoneData	  * m_pBoneData;

						NsRefBoneData	( NsAnim_BoneData * p ) { m_refCount = 0; m_pBoneData = p; }
						~NsRefBoneData	() { if ( m_pBoneData ) delete m_pBoneData; }

	void				addReference	( void ) { m_refCount++; }
	void				removeReference	( void ) { m_refCount--; }
	int					totalReferences	( void ) { return m_refCount; }
};

////////////////////////////////////////////////////////////

class NsRefBranchNodes
{
	int				m_refCount;
public:
	NsBranch	  * m_pBranchNodes;

					NsRefBranchNodes	( NsBranch * p ) { m_refCount = 0; m_pBranchNodes = p; }
					~NsRefBranchNodes	() { if ( m_pBranchNodes ) delete m_pBranchNodes; }

	void			addReference		( void ) { m_refCount++; }
	void			removeReference		( void ) { m_refCount--; }
	int				totalReferences		( void ) { return m_refCount; }
};

////////////////////////////////////////////////////////////

class NsRefLeafNodes
{
	int				m_refCount;
public:
	NsLeaf		  * m_pLeafNodes;

					NsRefLeafNodes	( NsLeaf * p ) { m_refCount = 0; m_pLeafNodes = p; }
					~NsRefLeafNodes	() { if ( m_pLeafNodes ) delete m_pLeafNodes; }

	void			addReference		( void ) { m_refCount++; }
	void			removeReference		( void ) { m_refCount--; }
	int				totalReferences		( void ) { return m_refCount; }
};

////////////////////////////////////////////////////////////

class NsRefTriangleMap
{
	int				m_refCount;
public:
	unsigned int  * m_pTriangleMap;

					NsRefTriangleMap	( unsigned int * p ) { m_refCount = 0; m_pTriangleMap = p; }
					~NsRefTriangleMap	() { if ( m_pTriangleMap ) delete m_pTriangleMap; }

	void			addReference		( void ) { m_refCount++; }
	void			removeReference		( void ) { m_refCount--; }
	int				totalReferences		( void ) { return m_refCount; }
};

////////////////////////////////////////////////////////////

#endif		// _REFTYPES_H_

/*
NsRefDouble
NsRefAcc
NsRefPair
NsRefSkinBox
NsRefCAS16
NsRefCAS32
NsRefFlipPairs
NsRefPartChecksums
NsRefBoneData
NsRefTexMan
*/