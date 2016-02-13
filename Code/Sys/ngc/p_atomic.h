//#ifndef _ATOMIC_H_
//#define _ATOMIC_H_
//
//#include "p_hw.h"
//#include "p_model.h"
//#include "p_frame.h"
//#include "p_anim.h"
//#include "p_reftypes.h"
//
//class NsAtomic
//{
//	NsMaterialMan		  * m_pMaterial;
//	NsFrame				  * m_pFrame;
//	// Animation weight data, stage 1 - paired blending.
//	NsRefDoubleData		  * m_pDoubleData;
//	// Animation weight data, stage 2 - accumulative blending.
//	NsRefAccData		  * m_pAccData;
//	// Animation vertex/normal pool.
//	int						m_numVertices;
//	NsRefTransformedVertices* m_pGeometryArrays;
//	NsRefTransformedVertices* m_pTransformedVertices;
//	NsAnim_PosNormPair	  * m_pTransformedVertices2;
//	NsAnim_PosNormPair	  * m_pTransformedVertices3;
//	int						m_lastTransform;
//	int						m_stride;
//
//	unsigned int			m_numSkinBoxes;
//	unsigned int			m_numCAS16;
//	unsigned int			m_numFlipPairs;
//	unsigned int			m_numCAS32;
//	unsigned int			m_numPartChecksums;
//
//	NsRefSkinBox		  * m_pSkinBox;
//	NsRefCAS16			  * m_pCAS16;
//	NsRefFlipPairs		  * m_pFlipPairs;
//	NsRefCAS32			  * m_pCAS32;
//	NsRefPartChecksums	  * m_pPartChecksums;
//	unsigned short			m_removeCAS16;
//	unsigned int			m_removeCAS32;
//
//	unsigned int			m_frameNumber;
//	NsAtomic			  * m_pNext;
//
//	void				  * m_pClump;
//
//	unsigned int			m_flags;	// Same as RpAtomicFlag.
//
//	// BSP data.
//	unsigned int			m_numBranchNodes;
//	unsigned int			m_numLeafNodes;
//	unsigned int			m_numTriangles;
//	NsRefBranchNodes	  * m_pBranchNodes;
//	NsRefLeafNodes		  * m_pLeafNodes;
//	NsRefTriangleMap	  * m_pTriangleMap;
//
//public:
//	friend class NsClump;
//	friend class NsModel;
//							NsAtomic		();
//							NsAtomic		( NsModel * pModelData, NsTextureMan * pTexMan );
//							~NsAtomic		();
//
//	NsModel				  * setModel		( NsModel * pModelData, NsTextureMan * pTexMan );
//
//	void					draw			( NsCamera * camera );
//	void					draw			( NsCamera * camera, ROMtx * pBoneMatrices, unsigned int transform );
//
//
//	NsFrame				  * getFrame		( void ) { return m_pFrame; };
//	void					setFrame		( NsFrame * p ) { m_pFrame = p; };
//
//	NsAtomic&				clone			( void );
//
//	NsAnim_SkinHeader	  * attachSkinData	( NsAnim_SkinHeader * pSkinHeader );
//
//	void				  * getClump		( void ) { return m_pClump; }
//
//	NsMaterialMan		  * getMaterials	( void ) { return m_pMaterial; }
//	void					setMaterials	( NsMaterialMan * p_mat ) { m_pMaterial = p_mat; }
//
//	unsigned int			getNumSkinBoxes		( void ) { return m_numSkinBoxes; }
//	unsigned int			getNumCASFlags16	( void ) { return m_numCAS16; }
//	unsigned int			getNumFlipPairs		( void ) { return m_numFlipPairs; }
//	unsigned int			getNumCASFlags32	( void ) { return m_numCAS32; }
//	unsigned int			getNumPartChecksums	( void ) { return m_numPartChecksums; }
//
//	NsBBox				  * getSkinBoxes		( void ) { return m_pSkinBox ? m_pSkinBox->m_pSkinBox : NULL; }
//	unsigned short		  * getCASFlags16		( void ) { return m_pCAS16 ? m_pCAS16->m_pCAS16 : NULL; }
//	unsigned int		  * getFlipPairs		( void ) { return m_pFlipPairs ? m_pFlipPairs->m_pFlipPairs : NULL; }
//	unsigned int		  * getCASFlags32		( void ) { return m_pCAS32 ? m_pCAS32->m_pCAS32 : NULL; }
//	unsigned int		  * getPartChecksums	( void ) { return m_pPartChecksums ? m_pPartChecksums->m_pPartChecksums : NULL; }
//	unsigned short			getCASRemoveFlags16	( void ) { return m_removeCAS16; }
//	unsigned int			getCASRemoveFlags32	( void ) { return m_removeCAS32; }
//
//	void					setFlags			( unsigned int flags ) { m_flags = flags; }
//	unsigned int			getFlags			( void ) { return m_flags; }
//
//	// BSP data.
//	unsigned int			getNumBranchNodes	( void ) { return m_numBranchNodes; }
//	unsigned int			getNumLeafNodes		( void ) { return m_numLeafNodes; }
//	unsigned int			getNumTriangles		( void ) { return m_numTriangles; }
//	NsBranch			  * getBranchNodes		( void ) { return m_pBranchNodes->m_pBranchNodes; }
//	NsLeaf				  * getLeafNodes		( void ) { return m_pLeafNodes->m_pLeafNodes; }
//	unsigned int		  * getTriangleMap		( void ) { return m_pTriangleMap->m_pTriangleMap; }
//
//};
//
//#endif		// _ATOMIC_H_





