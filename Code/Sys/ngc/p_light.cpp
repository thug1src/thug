//#include "p_light.h"
//#include "p_assert.h"
//
//unsigned int	NsLight::m_SlotArray			= 0;
//bool			NsLight::m_LightingHasChanged	= true;
//GXLightObj		NsLight::m_LightObj[NSLIGHT_MAX_LIGHTS];
//GXColor			NsLight::m_AmbientColor			= (GXColor){ 0, 0, 0, 0 };
//
//static GXColor	ColorOff = { 0, 0, 0, 0 };
//
//void NsLight::loadup( void )
//{
//	if( m_LightingHasChanged )
//	{
//		// Go through and load up all lights to the hardware.
//		for( int l = 0; l < NSLIGHT_MAX_LIGHTS; ++l )
//		{
//			if( m_SlotArray & ( 1 << l ))
//			{
//				GXLoadLightObjImm( &m_LightObj[l], (enum _GXLightID)( 1 << l ));
//			}
//		}
//	}
//	m_LightingHasChanged = false;
//}
//
//
//
//void NsLight::loadupAmbient( void )
//{
//	GX::SetChanAmbColor( GX_COLOR0A0, m_AmbientColor );
//}
//
//
//
//NsLight::NsLight( NsLightType type )
//{
//	assertp(( type == NSLIGHT_AMBIENT ) || ( type == NSLIGHT_DIRECTIONAL ));
//
//	m_Type		= type;
//	m_pFrame	= NULL;
//	m_Color.r	= 0;
//	m_Color.g	= 0;
//	m_Color.b	= 0;
//	m_Color.a	= 0;
//	m_On		= false;
//
//	activate( true );
//}
//
//
//
//NsLight::~NsLight()
//{
//	activate( false );
//}
//
//
//void NsLight::setFrame( NsFrame* p_frame )
//{
//	m_pFrame				= p_frame;
//	m_LightingHasChanged	= true;
//}
//
//
//
//void NsLight::setColor( GXColor color )
//{
//	m_Color.r = (int)( color.r * 255.0f );
//	m_Color.g = (int)( color.g * 255.0f );
//	m_Color.b = (int)( color.b * 255.0f );
//	m_Color.a = 0xFF;
//
//	updateColor();
//}
//
//
//
//void NsLight::updateColor( void )
//{
//	if( m_On )
//	{
//		if( m_Type == NSLIGHT_AMBIENT )
//		{
//			GX::SetChanAmbColor( GX_COLOR0A0, m_Color );
//			m_AmbientColor = m_Color;
//		}
//		else
//		{
//			m_LightingHasChanged = true;
//			GXInitLightColor( &m_LightObj[m_Slot], m_Color );
//		}
//	}
//}
//
//
//
//void NsLight::updateOrientation( void )
//{
//	// Prepares large number (2 pow 20) (taken from Nintendo demo code).
//	#define LARGE_NUMBER 1048576
//
//	if( m_pFrame && m_On && ( m_Type == NSLIGHT_DIRECTIONAL ))
//	{
//		m_LightingHasChanged = true;
//		float x_pos = -m_pFrame->getModelMatrix()->getAtX() * LARGE_NUMBER;
//		float y_pos = -m_pFrame->getModelMatrix()->getAtY() * LARGE_NUMBER;
//		float z_pos = -m_pFrame->getModelMatrix()->getAtZ() * LARGE_NUMBER;
//		GXInitLightPos( &m_LightObj[m_Slot], x_pos, y_pos, z_pos );
//	}
//}
//
//
//
//void NsLight::activate( bool on )
//{
//	if( on != m_On )
//	{
//		m_On = on;
//
//		if( m_Type == NSLIGHT_AMBIENT )
//		{
//			if( m_On )
//			{
//				GX::SetChanAmbColor( GX_COLOR0A0, m_Color );
//				m_AmbientColor = m_Color;
//			}
//			else
//			{
//				GX::SetChanAmbColor( GX_COLOR0A0, ColorOff );
//				m_AmbientColor = ColorOff;
//			}
//		}
//		else
//		{
//			m_LightingHasChanged = true;
//
//			if( m_On )
//			{
//				// Find a free slot.
//				for( int s = 0; s < NSLIGHT_MAX_LIGHTS; ++s )
//				{
//					if(( m_SlotArray & ( 1 << s )) == 0 )
//					{
//						m_Slot = s;
//						m_SlotArray |= ( 1 << s );
//
//						// In case orientation was changed whilst the light was off.
//						updateOrientation();
//						updateColor();
//						break;
//					}
//				}
//			}
//			else
//			{
//				// Turn the light off.
//				m_SlotArray &= ~( 1 << m_Slot );
//			}
//		}
//	}
//}








