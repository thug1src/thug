/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SYS Library												**
**																			**
**	Module:			SYS (SYS_) 												**
**																			**
**	File name:		keyboard.cpp											**
**																			**
**	Created:		03/08/2001	-	gj										**
**																			**
**	Description:	USB Keyboard interface									**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

#include <sys/sio/keyboard.h>

//#include <eekernel.h>
//#include <eeregs.h>
//#include <stdio.h>
//#include <sifdev.h>
//#include <sifrpc.h>
//
//#include <libusbkb.h>
extern "C"
{
int sceUsbKbInit(int *max_connect) { return 0; }
int sceUsbKbGetInfo(int *info) { return 0; }
int sceUsbKbRead(unsigned int no,int *data) { return 0; }
int sceUsbKbGetLocation(int no,unsigned char *location) { return 0; }
int sceUsbKbSetLEDStatus(int no, unsigned char led) { return 0; }
int sceUsbKbSetLEDMode(int no, int mode) { return 0; }
int sceUsbKbSetRepeat(int no, int sta_time, int interval) { return 0; }
int sceUsbKbSetCodeType(int no, int type) { return 0; }
int sceUsbKbSetArrangement(int no, int arrange) { return 0; }
int sceUsbKbSync(int mode, int *result) { return 0; }
}
/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace SIO
{

/*****************************************************************************
**								  Externals									**
*****************************************************************************/


/*****************************************************************************
**								Private Types								**
*****************************************************************************/


/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

#define USBKEYBD_ARG "keybd=8\0debug=1"

static	unsigned char *s_old_status = NULL;
//static	int	s_info;
//static	unsigned char 		s_location[7];
//static	int s_kdata;
static	int			s_max_connect;

enum
{
	vKEYCODE_RIGHT	=	0x804F,
	vKEYCODE_LEFT	=	0x8050,
	vKEYCODE_DOWN	=	0x8051,
	vKEYCODE_UP		=	0x8052,
	vKEYCODE_ESC	= 	0x8029,
};

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/



/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

//static void s_init_newkeyboard( int i );
//static int s_capture_input( int i, char* makes );

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int KeyboardInit(void)
{
	int result;
//    char *option = USBKEYBD_ARG;
	int i;

	//sceSifInitRpc(0);
	
//#ifdef __NOPT_CDROM__OLD
//	result = sceSifLoadModule(
//               "cdrom0:\\USBKB.IRX",
//                sizeof( USBKEYBD_ARG ) + 1, option );
//#else
//	result = sceSifLoadModule(
//               "host0:IOPModules/usbkb.irx",
//                sizeof( USBKEYBD_ARG ) + 1, option );
//#endif
	
//	if( result < 0 )
    {
        Dbg_MsgAssert( 0,( "EE:Can't load module usbkb.irx\n" ));
		return -1;
	}

    result = sceUsbKbInit( &s_max_connect );
//    if( result == USBKB_NG ) 
	{
		Dbg_Printf( "Initialize error\n" );
		return -1;
	}

    s_old_status = new unsigned char[ s_max_connect ];
    for( i = 0; i < s_max_connect; i++ ) 
	{ 
		s_old_status[i] = 0; 
	}

	return 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int KeyboardRead( char* makes )
{
//	int result;
//	int i, num_strokes;
//
//	// get keyboard info
//	result = sceUsbKbGetInfo( &s_info );
////	if( result != USBKB_OK ) 
//	{
//		Dbg_Printf( "Error%d : sceUsbKbGetInfo\n", result );
//		return 0;
//	}
////	sceUsbKbSync( USBKB_WAIT, &result );
////	if( result != USBKB_OK ) 
//	{
//		Dbg_Printf( "Error%d : sceUsbKbSync\n", result );
//		return 0;
//	}
//  
//	num_strokes = 0;
//	// reading keyboard
//	for( i = 0; i < s_max_connect; i++)  
//	{
////		if( s_info.status[i] == 0 ) 
//		{ 
//			continue; 
//		}  // not connected
//
//		if( s_old_status[i] == 0 ) 
//		{
//			Dbg_Printf( "New keyboard %d is connected\n", i );
//			s_init_newkeyboard(i);
//		}
//
//		result = sceUsbKbRead( i, &s_kdata );
////		if( result != USBKB_OK ) 
//		{
//			Dbg_Printf( "Error%d : sceUsbKbRead\n", result );
//			continue;
//		}
////		sceUsbKbSync( USBKB_WAIT, &result );
////		if( result != USBKB_OK ) 
//		{
//			Dbg_Printf( "Error%d : sceUsbKbSync\n",result);
//			continue;
//		}
//
////		if( s_kdata.len == 0 ) 
//		{
//			continue;
//		}
//
////		result = sceUsbKbGetLocation( i, s_location );
////		if( result != USBKB_OK ) 
//		{
//			Dbg_Printf( "Error%d : sceUsbKbGetLocation\n", result );
//			continue;
//		}
//		
////		sceUsbKbSync( USBKB_WAIT, &result );
////		if( result != USBKB_OK ) 
//		{
//			Dbg_Printf( "Error%d : sceUsbKbSync\n", result );
//			continue;
//		}
//		
//		num_strokes = s_capture_input( i, makes );
//	}
//  
//	for( i = 0; i < s_max_connect; i++ ) 
//	{ 
////		s_old_status[i] = s_info.status[i]; 
//	}
//
//	return num_strokes;
	return 0;
}

void KeyboardClear( void )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

//static void s_init_newkeyboard( int i )
//{       
//	int result;
//	
//	result = sceUsbKbSetLEDStatus( i, USBKB_LED_NUM_LOCK );
//	if( result != USBKB_OK ) 
//	{
//		Dbg_Printf( "Error%d : sceUsbKbSetLEDStatus\n", result );
//	} 
//	else 
//	{
//		sceUsbKbSync(USBKB_WAIT,&result);
//		if( result != USBKB_OK ) 
//		{
//			Dbg_Printf( "Error%d : sceUsbKbSync\n", result );
//		}
//	}
//	
//	result = sceUsbKbSetLEDMode( i, USBKB_LED_MODE_AUTO1 );
//	if( result != USBKB_OK ) 
//	{
//		Dbg_Printf( "Error%d : sceUsbKbSetLEDMode\n", result );
//	} 
//	else 
//	{
//		sceUsbKbSync( USBKB_WAIT, &result );
//		if( result != USBKB_OK ) 
//		{
//			Dbg_Printf( "Error%d : sceUsbKbSync\n", result );
//		}
//	}
//	
//	sceUsbKbSetRepeat( i, 30, 2 );
//	sceUsbKbSetCodeType( i, USBKB_CODETYPE_ASCII );
//	sceUsbKbSetArrangement( i, USBKB_ARRANGEMENT_106 );
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

//static int s_capture_input( int i, char* makes )
//{
//	unsigned short kcode;
//	int j, k, index;
//	
//	//Dbg_Printf( "usbkeybd%d : ", i );
//	
//	//Dbg_Printf( "port=" );
//	for( k = 0; k < 7 && s_location[k] != 0; k++ )
//	{
//		//Dbg_Printf( "%s%d", ((k)? ",": "" ), s_location[k] );
//	}
//	  
//	
//	//Dbg_Printf( " : LED[%02X] ", s_kdata.led );
//	
//	//Dbg_Printf( "MKEY[%02X] ", s_kdata.mkey );
//	
//	//Dbg_Printf( "KEY[" );
//	
//	index = 0;
//	for( j=0; j < s_kdata.len; j++ ) 
//	{
//		kcode = s_kdata.keycode[j];
//		if( kcode & USBKB_RAWDAT ) 
//		{
//			//Dbg_Printf( "%04X ", kcode );
//			switch( kcode )
//			{   
//				case vKEYCODE_RIGHT:
//					makes[index++] = vKB_RIGHT;
//					break;
//				case vKEYCODE_LEFT:
//					makes[index++] = vKB_LEFT;
//					break;
//				case vKEYCODE_DOWN:
//					makes[index++] = vKB_DOWN;
//					break;
//				case vKEYCODE_UP:
//					makes[index++] = vKB_UP;
//					break;
//				case vKEYCODE_ESC:
//					makes[index++] = vKB_ESCAPE;
//					break;
//			}
//			continue;
//		}
//		if( kcode & USBKB_KEYPAD ) 
//		{
//			// 10 key
//			if((kcode & 0x00ff) == '\n')
//			{ 
//				//Dbg_Printf( "(\\n)" ); 
//				makes[index++] = vKB_ENTER;
//				continue;
//			}
//			//Dbg_Printf( "(%c) ", kcode & ~USBKB_KEYPAD );
//			makes[index++] = kcode & ~USBKB_KEYPAD;
//			continue;
//		}
//		// Normal key
//		if((kcode & 0x00ff) == '\0' ) 
//		{ 
//			//Dbg_Printf( "\'\\0\'" ); 
//			continue;
//		}
//		if((kcode & 0x00ff) == '\n' ) 
//		{ 
//			//Dbg_Printf( "\'\\n\'" ); 
//			makes[index++] = vKB_ENTER;
//			continue;
//		}
//		if((kcode & 0x00ff) == '\t' ) 
//		{ 
//			//Dbg_Printf( "\'\\t\'" ); 
//			continue;
//		}
//		if((kcode & 0x00ff) == '\b' ) 
//		{ 
//			//Dbg_Printf( "\'\\b\'" ); 
//			makes[index++] = vKB_BACKSPACE;
//			continue;
//		}
//		//Dbg_Printf( "\'%c\' ", kcode );
//		makes[index++] = kcode;
//	}
//	//Dbg_Printf( "]\n" ); 
//
//	return index;
//	return 0;
//}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace SIO
