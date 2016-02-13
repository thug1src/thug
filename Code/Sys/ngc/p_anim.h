//#ifndef _ANIM_H_
//#define _ANIM_H_
//
//#include "p_hw.h"
//#include "p_quat.h"
//#include "p_matrix.h"
//
//typedef struct {
//	NsVector	v;
//	float		time;
//} NsTFrame;
//
//typedef struct {
//	NsQuat	q;
//	float	time;
//} NsQFrame;
//
//typedef struct {
//	unsigned int	id;
//	unsigned int	numQFrames;
//	unsigned int	numTFrames;
//	unsigned int	numFrames;
//	unsigned int	flags;
//	float			duration;
//	unsigned int	pad[2];
//} NsAnim_Sequence;
//
//typedef struct {
//	unsigned int	id;
//	unsigned int	numBones;
//	unsigned int	pad[6];
//} NsAnim_BoneHeader;
//
//typedef struct {
//	float			rightx;
//	float			righty;
//	float			rightz;
//	unsigned int	matrixFlags;
//	float			upx;
//	float			upy;
//	float			upz;
//	unsigned int	boneIndex;
//	float			atx;
//	float			aty;
//	float			atz;
//	unsigned int	boneTag;
//	float			posx;
//	float			posy;
//	float			posz;
//	unsigned int	flags;
//} NsAnim_BoneData;
//
//typedef struct {
//	unsigned int	id;
//	unsigned int	numVerts;
//	unsigned int	pad[6];
//} NsAnim_SkinHeader;
//
//typedef struct {
//	unsigned char	index[4];
//	float			weight[4];
//} NsAnim_Weight;
//
//typedef struct {
//	float pos[3];
//	float norm[3];
//} NsAnim_PosNormPair;
//
//typedef struct {
//	float w[2];
//} NsAnim_WeightPair;
//
//class NsAnim
//{
//	NsQFrame		  * m_q0;	// Malloc size ( QFrame ) * numBones
//	NsTFrame		  * m_t0;	// Malloc size ( TFrame ) * numBones
//	NsQFrame		  * m_q1;	// Malloc size ( QFrame ) * numBones
//	NsTFrame		  * m_t1;	// Malloc size ( TFrame ) * numBones
//	NsQFrame		  * m_qnow;	// Malloc size ( QFrame ) * numBones
//	NsTFrame		  * m_tnow;	// Malloc size ( TFrame ) * numBones
//	NsQFrame		  * m_pQ0;
//	NsTFrame		  * m_pT0;
//	NsQFrame		  * m_pQ1;
//	NsTFrame		  * m_pT1;
//	NsAnim_Sequence	  * m_pCurrentSequence;
//	int					m_numCachedBones;
//	float				m_duration;
//	float				m_cachedTime;
////	ROMtx			  * m_pBoneMat;
//	float				m_time;
//
//	void				reset				( void );
//	void				getQuat				( unsigned int bone, NsQuat * pQuat );
//	void				getTrans			( unsigned int bone, float * pX, float * pY, float * pZ );
//public:
//
//						NsAnim				( unsigned int numBones );
//						NsAnim				( NsAnim_Sequence * pAnim, unsigned int numBones );
//
//						~NsAnim				();
//
//	void				setCurrentSequence	( NsAnim_Sequence * pSequence );
//
//	void				update				( void );
//	void				load				( char * pFilename );
//
//	void				buildKeys			( void );
////	void				flipBoneMatrices	( ROMtx* p_matrices, unsigned int num_flip_pairs, unsigned int* p_flip_pairs );
////	ROMtx			  * buildBoneMatrices	( NsAnim_BoneData * pBoneData );
//
//	NsAnim&				clone				( void );
//
//	int					numBones			( void ) { return m_numCachedBones; }
//
//	void				setTime				( float time ) { m_time = time; }
//	float				getTime				( void ) { return m_time; }
//	NsQFrame*			getQFrameNow		( void ) { return m_qnow; }
//	NsTFrame*			getTFrameNow		( void ) { return m_tnow; }
//};
//
//#endif		// _ANIM_H_




















