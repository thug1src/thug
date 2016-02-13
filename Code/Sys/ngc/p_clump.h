//#ifndef _CLUMP_H_
//#define _CLUMP_H_
//
//#include "p_hw.h"
//#include "p_camera.h"
//#include "p_texman.h"
//#include "p_matman.h"
//#include "p_dlman.h"
//#include "p_model.h"
//#include "p_frame.h"
//#include "p_atomic.h"
//#include "p_anim.h"
//#include "p_reftypes.h"
//
//typedef NsAtomic * (*NsClump_Callback)( NsAtomic * clump, void * data );
//
//class NsClump
//{
//	static const int	NUM_BLEND_ANIMS	= 3;			// Max number of blended anims.
//
//	NsFrame			  *	m_pFrame;
//	NsAtomic		  * m_pAtomicHead;
//	int					m_numAtomic;
//
//	ROMtx			  * m_pBoneMat;
//	NsRefBoneData	  * m_pBoneData;
//	NsAnim			  * m_pAnim[NUM_BLEND_ANIMS];
//	float				m_AnimWeights[NUM_BLEND_ANIMS];	// Weight per anim for blending.
//	bool				m_AnimFlipped;
//
//	unsigned int		m_numBones;
//
//	NsFrame			  *	m_pFrameList;
//	int					m_numFrames;
//
//	NsClump			  * m_pNext;
//
//	void			  * m_pWorld;
//	void			  * m_pUserData;
//
//	NsTextureMan	  * m_pTexMan;
//public:
//	friend class NsScene;
//
//						NsClump				();
//						NsClump				( NsModel * pModelData );
//						NsClump				( unsigned int * pDFF );
//						~NsClump			();
//
//	NsClump&			clone				( void );
//	void				merge				( NsClump& source );
//
//	NsModel			  * setModel			( NsModel * pModelData );
//	void				setAtomics			( unsigned int * pDFF );
//	void			  * getUserData			( void )			{ return m_pUserData; }
//	void				setUserData			( void * p_data )	{ m_pUserData = p_data; }
//
//	void				addAtomic			( NsAtomic * pAtomic );
//	void				removeAtomic		( NsAtomic * pAtomicToRemove );
//
//	void				draw				( NsCamera * camera, unsigned int transform );
//
//	NsFrame			  * getFrame			( void ) { return m_pFrame; };
//	void				setFrame			( NsFrame * p ) { m_pFrame = p; };
//
//	void				flipAnim			( bool flip )	{ m_AnimFlipped = flip; };
//	void				setAnimWeights		( float* p_weights, int num_weights );
//	void				buildBoneMatrices	( NsQFrame* p_q_frames, NsTFrame* p_t_frames );
//	void				flipBoneMatrices	( void );
//	void				processAnims		( void );
//	void				getHookPosition		( NsVector& hook_in, unsigned int bone, NsVector& hook_out );
//
//	void				forAllAtomics		( NsClump_Callback pCB, void * pData );
//
//	NsAnim			  * getAnim				( int index )	{ return m_pAnim[index]; };
//
//	void			  * getWorld			( void ) { return m_pWorld; }
//
//	int					getNumAtomics		( void ) { return m_numAtomic; }
//	unsigned int		getNumBones			( void ) { return m_numBones; }
//
//	void				removeCASPolys		( unsigned int flags );
//
//	void				setTexMan			( NsTextureMan * pTexMan ) { m_pTexMan = pTexMan; }
//	NsTextureMan	  * getTexMan			( void ) { return m_pTexMan; }
//};
//
//#endif		// _CLUMP_H_

























