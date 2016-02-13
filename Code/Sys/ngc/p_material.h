//#ifndef _MATERIAL_H_
//#define _MATERIAL_H_
//
//#include "p_hw.h"
//#include "p_dl.h"
//#include "p_render.h"
//#include "p_tex.h"
//
//typedef struct {
//	unsigned int	time;
//	GXColor			color;
//} NsWibbleKey;
//
//typedef struct {
//	NsWibbleKey	  * pKeys;
//	int				numKeys;
//} NsWibbleSequence;
//
//class NsMaterial
//{
//	unsigned int	m_id;				// 4
//	unsigned int	m_version;			// 4
//	unsigned int	m_number;			// 4 - Number instead of hash value.
//	NsTexture	  * m_pTexture;			// 4
//	unsigned int	m_nDL;				// 4
//	NsDL		  * m_pDL;				// 4
//	NsBlendMode		m_blendMode;		// 4
//	GXColor			m_color;			// 4
//	unsigned char	m_alpha;			// 1
//	unsigned char	m_type;				// 1
//	unsigned char	m_flags;			// 1
//	unsigned char	m_UVWibbleEnabled;	// 1
//	unsigned int	m_priority;			// 4
//	NsWibbleSequence * m_pWibbleData;	// 4
//	float			m_uvel;				// 4
//	float			m_vvel;     		// 4
//	float			m_uamp;     		// 4
//	float			m_vamp;     		// 4
//	float			m_uphase;			// 4
//	float			m_vphase;			// 4
//	float			m_ufreq;    		// 4
//	float			m_vfreq;    		// 4
//	NsTexture_Wrap	m_wrap;				// 4
//										// Total: 84
//
//	bool				getUVWibbleParameters	( float* u_offset, float* v_offset, int pass );
//	GXColor*			getVCWibbleParameters	( NsDL* p_dl, char** change_mask );
//
//	friend class NsMaterialMan;
//	friend class NsModel;
//public:
//	void				init		( unsigned int number );
//
//	void				addDL		( NsDL * pDLToAdd );
//
//	void				deleteDLs	( void );
//
//	void				draw		( void );
//
//	int					numDL		( void ) { return m_nDL; }
//	NsDL			  * headDL		( void ) { return m_pDL; }
//
//	unsigned char		getType		( void ) { return m_type; }
//	void				setType		( unsigned char type ) { m_type = type; }
//
//	unsigned char		getFlags	( void ) { return m_flags; }
//	void				setFlags	( unsigned char flags ) { m_flags = flags; }
//
//	// Note: these were added for fast building of new materials for shadow rendering, and should be used
//	// with care elsewhere...
//	void				setColor	( GXColor color )			{ m_color = color; }
//	GXColor				getColor	( void )					{ return m_color; }
//	void				setTexture	( NsTexture * pTexture )	{ m_pTexture = pTexture; }
//	NsTexture*			getTexture	( void )					{ return m_pTexture; }
//	void				setnDL		( unsigned int nDL )		{ m_nDL = nDL; }
//	unsigned int		getnDL		( void )					{ return m_nDL; }
//	void				setpDL		( NsDL * pDL )				{ m_pDL = pDL; }
//	NsDL*				getpDL		( void )					{ return m_pDL; }
//	NsBlendMode			getBlendMode( void )					{ return m_blendMode; }
//	void				setBlendMode( NsBlendMode mode )		{ m_blendMode = mode; }
//	void				setAlpha	( unsigned char alpha )		{ m_alpha = alpha; }
//	bool				wibbleUV	( NsDL* p_dl );
//	bool				wibbleVC	( NsDL* p_dl );
//
//	NsWibbleSequence*	getWibbleData	( void )						{ return m_pWibbleData; }
//	void				setWibbleData	( NsWibbleSequence* p_data )	{ m_pWibbleData = p_data; }
//
//	void				setWrapMode		( NsTexture_Wrap wrap )			{ m_wrap = wrap; }
//	NsTexture_Wrap		getWrapMode		( void ) { return m_wrap; }
//};
//
//#endif		// _MATERIAL_H_
