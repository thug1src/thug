//#ifndef _P_LIGHT_H_
//#define _P_LIGHT_H_
//
//#include "p_hw.h"
//#include "p_frame.h"
//
//enum NsLightType
//{
//	NSLIGHT_UNKNOWM	= 0,
//	NSLIGHT_DIRECTIONAL,
//    NSLIGHT_AMBIENT,
//};
//
//#define NSLIGHT_MAX_LIGHTS	4
//
//class NsLight
//{
//	static unsigned int	m_SlotArray;
//	static GXLightObj	m_LightObj[NSLIGHT_MAX_LIGHTS];
//	static bool			m_LightingHasChanged;
//	static GXColor		m_AmbientColor;
//	
//	NsLightType			m_Type;
//	NsFrame*			m_pFrame;
//	GXColor				m_Color;	
//	NsFrame				m_Frame;
//	bool				m_On;
//	int					m_Slot;
//
//	public:
//
//	static void			loadup( void );
//	static void			loadupAmbient( void );
//	static GXLightID	getLightID( void )				{ return (GXLightID)m_SlotArray; }
//
//						NsLight( NsLightType type );
//						~NsLight();
//
//	void				setFrame( NsFrame* p_frame );
//	void				setColor( GXColor color );
//	void				activate( bool on );
//	void				updateOrientation( void );
//	void				updateColor( void );
//};
//
//#endif _P_LIGHT_H_







