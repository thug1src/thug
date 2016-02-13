/*	Useful little macros...
	
	Add any macros you want into this:
*/

#ifndef __USEFUL_LITTLE_MACROS_H__
#define __USEFUL_LITTLE_MACROS_H__

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <core/support.h>

#define PERCENT_MULT			( ( 1.0f ) / 100.0f )
#define PERCENT( x, percent )	( ( ( ( ( float )( x ) ) * ( ( float )( percent ) ) ) * PERCENT_MULT ) )

#define		IPU			(1.0f)			// Inches per unit					
																
#define		INCHES(x)						((float)(x))
#define		INCHES_PER_SECOND(x)			((float)(x))
#define		INCHES_PER_SECOND_SQUARED(x)	((float)(x))

#define 	FEET_TO_INCHES( x )				( x * 12.0f )
#define		FEET(x)							((float)(x*12.0f))					  

#define 	FEET_PER_MILE					5280.0f
#define 	MPH_TO_INCHES_PER_SECOND( x )	( ( ( x ) * FEET_PER_MILE * 12.0f ) / ( 60.0f * 60.0f ) )
#define		MPH_TO_IPS( x )					MPH_TO_INCHES_PER_SECOND( x )
#define		INCHES_PER_SECOND_TO_MPH( x )	( ( x ) / 12.0f / FEET_PER_MILE * 60.0f * 60.0f )
#define		IPS_TO_MPH( x )					INCHES_PER_SECOND_TO_MPH( x )

#define		RADIANS_PER_SECOND_TO_RPM( x )	(9.5496f * ( x ))
#define		RPM_TO_RADIANS_PER_SECOND( x )	(0.10472f * ( x ))

#define		DEGREES_TO_RADIANS( x ) 		( ( x ) * ( 2.0f * Mth::PI / 360.0f ) )
#define		RADIANS_TO_DEGREES( x ) 		( ( x ) / ( DEGREES_TO_RADIANS( 1.0f ) ) )
#define		THE_NUMBER_OF_THE_BEAST			666



// Takes screen coordinates in 'default' screen space - 640x448 - and converts them based on PAL and system defines.

// Convert from logical to SCREEN coordinates
#ifdef __PLAT_XBOX__
// Unfortunately, this has to be a platform-specific macro, since it references Xbox specific global structures to determine
// the width and height of the back buffer.
#include <gfx/xbox/nx/nx_init.h>
#define		SCREEN_CONV_X( x )				((( x ) * NxXbox::EngineGlobals.screen_conv_x_multiplier ) + NxXbox::EngineGlobals.screen_conv_x_offset )
#define		SCREEN_CONV_Y( y )				((( y ) * NxXbox::EngineGlobals.screen_conv_y_multiplier ) + NxXbox::EngineGlobals.screen_conv_y_offset )
#else
#define		SCREEN_CONV_X( x )				(Config::GetHardware()==Config::HARDWARE_XBOX ? (int)((( x ) * ( 640.0f / 704.0f )) + 32 )	: Config::PAL() ? (( x ) * 512 ) / 640 : (x) )
#define		SCREEN_CONV_Y( y )				(Config::GetHardware()==Config::HARDWARE_XBOX ? (( y ) + 16 )								: Config::PAL() ? (( y ) * 512 ) / 448 : (y) )
#endif // __PLAT_XBOX__

// Convert from screen to LOGICAL coordinates
#define		LOGICAL_CONV_X( x )				(Config::GetHardware()==Config::HARDWARE_XBOX ? (x) : Config::PAL() ? (( x ) * 640 ) / 512 : (x) )
#define		LOGICAL_CONV_Y( y )				(Config::GetHardware()==Config::HARDWARE_XBOX ? (y) : Config::PAL() ? (( y ) * 448 ) / 512 : (y) )

#endif // __USEFUL_LITTLE_MACROS_H__

