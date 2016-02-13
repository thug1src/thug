//#ifndef _SCENE_H_
//#define _SCENE_H_
//
//#include "p_hw.h"
//#include "p_camera.h"
//#include "p_texman.h"
//#include "p_matman.h"
//#include "p_dlman.h"
//#include "p_model.h"
//#include "p_clump.h"
//#include "p_matrix.h"
//
//typedef struct {
//	NsVector		emPos;
//
//	NsVector		emRotRight;		// Same as RwMatrix
//	unsigned int	emRotFlags;
//	NsVector		emRotUp;
//	unsigned int	emRotPad1;
//	NsVector		emRotAt;
//	unsigned int	emRotPad2;
//	NsVector		emRotPos;
//	unsigned int	emRotPad3;
//
//	float			emWidth;
//	float			emHeight;
//	float			emAngle;
//	unsigned int	emNumParticles;
//
//	float			partLifeMin;
//	float			partLifeRange;
//	unsigned int	partStartColor;
//	unsigned int	partEndColor;
//	float			partAR;
//	float			partWidth;
//	NsVector		partForce;
//	float			partGrowth;
//	float			partSpeedMin;
//	float			partSpeedRange;
//	float			partSpeedDamping;
//
//	char			textureName[64-8];
//	unsigned char	type;
//	unsigned char	flags;
//	unsigned char	alpha;
//	unsigned char	blendmode;
//	unsigned int	color;
//	unsigned int	nameChecksum;
//} NsParticle;
//
//typedef NsClump * (*NsScene_Callback)( NsClump * clump, void * data );
//
//class NsScene
//{
//	NsMatrix		m_view;
//
//	NsClump		  * m_pClumpHead;
//	unsigned int  * m_pCameraData;
//	unsigned int	m_numParticles;
//	NsParticle	  * m_pParticleData;
//
//	NsTextureMan  * m_pTexMan;
//
//	NsCollision	  * m_pCollision;
//public:
//	void		  * m_SSMan;
//	NsCull_Item		m_cull;			// Describes size of world.
//	NsMaterialMan *	m_pMaterial;
//
//					NsScene			();
//					~NsScene		();
//
//	void			loadBSP			( const char * pFilename );
//
//	void			draw			( NsCamera * camera );
//
//	void			addClump		( NsClump * pClumpToAdd );
//	void			removeClump		( NsClump * pClumpToRemove );
//
//	unsigned int  *	getCameraData	( void ) { return m_pCameraData; };
//
//	void			forAllClumps	( NsScene_Callback pCB, void * pData );
//
//	void			setTexMan		( NsTextureMan * pTexMan ) { m_pTexMan = pTexMan; }
//	NsTextureMan  * getTexMan		( void ) { return m_pTexMan; }
//
//	void			setParticleData	( NsParticle * pParticleData ) { m_pParticleData = pParticleData; }
//	unsigned int	getNumParticles ( void ) { return m_numParticles; }
//	NsParticle	  * getParticleData ( void ) { return m_pParticleData; }
//
//	int				findCollision	( NsLine * pLine, NsDL::Collision_LineCallback pCb, void * pData );
//};
//
//#endif		// _SCENE_H_
