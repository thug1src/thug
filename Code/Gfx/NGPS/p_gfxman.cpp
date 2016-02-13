/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GFX (Graphics Library)									**
**																			**
**	Module:			Graphics (GFX)		 									**
**																			**
**	File name:		p_gfxman.cpp											**
**																			**
**	Created:		07/26/99	-	mjb										**
**																			**
**	Description:	Graphics device manager									**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/
		
extern "C"				 
{

#include <sifdev.h>
#include <libgraph.h>
#include <libpkt.h>
}

#include <gfx/gfxman.h>
#include <gfx/nxviewport.h>
#include <sys/file/filesys.h>

#include <gel/scripting/symboltable.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

#define NUMPIXELS ( width * height )

#define HEADSIZE 54
#define IMAGESIZE ( NUMPIXELS * 3 )
#define BITMAPSIZE ( IMAGESIZE + HEADSIZE )

// Macro to store 32-bit data to unaligned address.
#define SET_UINT32( addr, x ) ( \
	*(( uint16 * )( addr )) = ( x ), \
	*(( uint16 * )(( addr ) + 2 )) = (( x ) >> 16 ) \
)

void SetupBMPHeader(uint8 *bmp, int width, int height)
{
	// Initialize the BMP header structure.
	// Bitmapfileheader
	*bmp						= 'B';              // Type
	*( bmp + 1 )				= 'M';
	SET_UINT32( bmp + 2, BITMAPSIZE );				// Size
	*( uint16 * )( bmp + 6 )	= 0;                // Reserved
	SET_UINT32( bmp + 10, HEADSIZE );				// Offset

	// Bitmapinfoheader
	SET_UINT32( bmp + 14, 40 );						// Bitmapinfoheader size
	SET_UINT32( bmp + 18, width );					// Width
	SET_UINT32( bmp + 22, height );					// Height
	*( uint16 * )( bmp + 26 )	= 1;      			// Planes
	*( uint16 * )( bmp + 28 )	= 24;     			// Bitcount
	SET_UINT32( bmp + 30, 0 );						// Compression
	SET_UINT32( bmp + 34, IMAGESIZE );				// Image size in bytes
	SET_UINT32( bmp + 38, 4740 );					// X Pels per metre
	SET_UINT32( bmp + 42, 4740 );					// Y Pels per metre
	SET_UINT32( bmp + 46, 0 );						// ClrUsed
	SET_UINT32( bmp + 50, 0 );						// ClrImportant

}


//--------------------------------------------------------------------------------------------
// File: main.c
// Date: August 19, 2000
// Author: George Bain @ Sony Computer Entertainment Europe
// Description: Store GS local memory -> Main/SPR memory 
// Notes: - read StoreTextureVIF1() and DumpImage() for more information
//        - pause screen using "square + cross buttons" to avoid blur
//
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// I N C L U D E S
//--------------------------------------------------------------------------------------------

#include <eekernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <libdev.h>
#include <eeregs.h>
#include <libgraph.h>
#include <libdma.h>
#include <libvu0.h>
#include <sifdev.h>
#include <libpkt.h>
#include <sifdev.h>
#include <sifrpc.h>
#include <libpad.h>

#include	<gfx\ngps\nx\dma.h>


//--------------------------------------------------------------------------------------------
// D E F I N E S
//--------------------------------------------------------------------------------------------

#define SCRN_W (640)
#define SCRN_H (224)
#define SCRN_CENTER_X (2048.0f)
#define SCRN_CENTER_Y (2048.0f)
#define SCRN_Z (512.0f)
#define ZBUF_MAX (16777000.0f)
#define ZBUF_MIN (1.0f)
#define ZCLIP_MAX (16777216.0f)
#define ZCLIP_MIN (1.0f)
#define ASPECT_X (1.0f)
#define ASPECT_Y (((float)SCRN_H*4.0f)/((float)SCRN_W*3.0f))
#define STRING_SIZE   (64)
#define OFFX (((4096-SCRN_W)/2)<<4)
#define OFFY (((4096-SCRN_H)/2)<<4)
#define PI (3.141592f)

#define RAD_TO_DEG(x) (x * 180.0f / PI)
#define DEG_TO_RAD(x) (x * PI / 180.0f)

#define ROT_SPEED (1.0f)
#define SET_VECTOR(_p,_x,_y,_z,_w)  ((_p)[0] = _x, (_p)[1] = _y, (_p)[2] = _z, (_p)[3] = _w)
#define TRANS (10.0f)
#define SPR_MEM (0x70000000)
#define UNCACHED_MEM (0x20000000)
#define DMA_SPR (0x80000000)
#define DMA_MEM (0x0FFFFFFF)
#define CUBE_VERT (36)

#define GIFTAG_NLOOP (CUBE_VERT)
#define GS_PRIM_SHADED (1)
#define GS_PRIM_ALPHA (1)
#define GIFTAG_EOP (1)
#define GIFTAG_PRE (1)
#define GS_PRIM_TEX (1)

#define RGB_FRAME (1)

#define GIFTAG_MAX_NLOOP (32767)
#define IMAGE_W (256)
#define IMAGE_H (256)
#define STORE_IMAGE_W (SCRN_W)
#define STORE_IMAGE_H (SCRN_H)
#define IMAGE_ADDR ( ((SCRN_W*SCRN_H*4)*3) / 256 )
#define CLUT_CSM1_MODE (0)
#define CLUT_CSM2_MODE (1)

//--------------------------------------------------------------------------------------------
// S T R U C T U R E S
//--------------------------------------------------------------------------------------------

typedef struct rect_tag
{

  short x, y;
  short w, h;

}
RECT;

typedef struct timimage_tag
{

  u_int mode;
  u_int id;
  u_int flag;
  u_int cbnum;
  RECT crect;
  u_int *caddr;
  u_int pbnum;
  RECT prect;
  u_int *paddr;

}
TIM_IMAGE;


//--------------------------------------------------------------------------------------------
// G L O B A L S
//--------------------------------------------------------------------------------------------

sceVu0FVECTOR camera_p = { 0.0f, 0.0f, -300.0f, 0.0f };
sceVu0FVECTOR camera_zd = { 0.0f, 0.0f, 1.0f, 1.0f };
sceVu0FVECTOR camera_yd = { 0.0f, 1.0f, 0.0f, 1.0f };
sceVu0FVECTOR light0 = { 1.0f, 0.0f, 0.3f, 0.0f };
sceVu0FVECTOR light1 = { 0.0f, 1.0f, 0.3f, 0.0f };
sceVu0FVECTOR light2 = { 0.0f, 0.0f, 1.0f, 0.0f };
sceVu0FVECTOR color0 = { 0.3f, 0.3f, 0.3f, 1.0f };
sceVu0FVECTOR color1 = { 0.3f, 0.3f, 0.3f, 1.0f };
sceVu0FVECTOR color2 = { 0.4f, 0.4f, 0.4f, 1.0f };
sceVu0FVECTOR ambient = { 0.2f, 0.2f, 0.2f, 0.0f };
sceVu0FVECTOR obj_trans = { 0.0f, 0.0f, 0.0f, 0.0f };
sceVu0FVECTOR obj_rot = { 0.0f, 0.0f, 0.0f, 0.0f };
sceVu0FMATRIX local_world;
sceVu0FMATRIX world_view;
sceVu0FMATRIX view_screen;
sceVu0FMATRIX local_screen;
sceVu0FMATRIX normal_light;
sceVu0FMATRIX light_color;
sceVu0FMATRIX local_light;
sceVu0FMATRIX work;
sceGsDBuff db;
sceDmaChan *dmaGIF = NULL;
sceDmaChan *dmaVIF1 = NULL;
float delta = ROT_SPEED;
u_int paddata;
u_char rdata[32];
u_long128 pad_dma_buf[scePadDmaBufferMax] __attribute__ ( ( aligned( 64 ) ) );
TIM_IMAGE texture;


//--------------------------------------------------------------------------------------------
// P R O T O T Y P E S
//--------------------------------------------------------------------------------------------

void InitPad( void );
void InitSystem( void );
void InitGraphics( void );
void ControlInput( void );
void SetAlphaBlend( void );
void SetTexFilter( void );
void SetTextureInfo( void );
void LoadTexture( u_long128 * base_addr, short pixel_mode, short addr, short w, short h,
		  short dest_x, short dest_y );
void ClearVRAM( u_char r, u_char g, u_char b, u_char a );
void CubePacket( sceVu0FVECTOR * vertex, sceVu0FVECTOR * normal, sceVu0FVECTOR * color,
		 sceVu0FVECTOR * st );
void sceVu0NormalColorVector( sceVu0IVECTOR c0, sceVu0FMATRIX local_light,
			      sceVu0FMATRIX light_color, sceVu0FVECTOR v0, sceVu0FVECTOR c1 );
void ReadTIM( u_char * file, TIM_IMAGE * tim, short clut_store_mode );
void StoreTextureVIF1( u_long128 * base_addr, short start_addr, short pixel_mode, short x,
		       short y, short w, short h, short frame_width );
void DumpImage( int frame_cnt, int oddeven );


//--------------------------------------------------------------------------------------------
// Function: DumpImage()
// Description: Uses StoreTextureVIF1() to grab both the odd and even frame buffers in GS local
//              memory.  A 24-bit TIM image is written to a file called "image.tim".
// Paramaters:  int frame_cnt: odd or even frame
//              int oddeven: odd or even frame (Vsync)
// Returns: none
// Notes: N/A
//--------------------------------------------------------------------------------------------

/*
void DumpImage( int frame_cnt, int oddeven )
{

  u_long128 *dst1 = NULL, *dst2 = NULL;
  int fd;
  int cnt = 0;
  int index = 0;
  u_int bnum __attribute__ ( ( aligned( 16 ) ) );
  u_short pixel_head[4] __attribute__ ( ( aligned( 16 ) ) );
  u_int header[3] __attribute__ ( ( aligned( 16 ) ) );

  // create image file
  fd = sceOpen( "host:image.tim", SCE_WRONLY | SCE_TRUNC | SCE_CREAT );

  header[0] = 0x10;		// id
  header[1] = 0x03;		// flag 2= 16bit 3 = 24bit

  // write header
  sceWrite( fd, &header[0], 4 );
  sceWrite( fd, &header[1], 4 );

  bnum = ( ( STORE_IMAGE_W * STORE_IMAGE_H * 3 ) * 2 ) + 12;	// BNUM
  pixel_head[0] = 0;		//x
  pixel_head[1] = 0;		//y
  pixel_head[2] = 960;		//w
  pixel_head[3] = 448;		//h

  // write pixel header information
  sceWrite( fd, &bnum, 4 );
  sceWrite( fd, &pixel_head[0], 2 );
  sceWrite( fd, &pixel_head[1], 2 );
  sceWrite( fd, &pixel_head[2], 2 );
  sceWrite( fd, &pixel_head[3], 2 );

  // allocate some memory
  dst1 = ( u_long128 * ) memalign( 64, STORE_IMAGE_W * STORE_IMAGE_H * 3 );
  dst2 = ( u_long128 * ) memalign( 64, STORE_IMAGE_W * STORE_IMAGE_H * 3 );

  StoreTextureVIF1( dst1, 0, SCE_GS_PSMCT24, 0, 0, STORE_IMAGE_W, STORE_IMAGE_H, SCRN_W );
  StoreTextureVIF1( dst2, 2240, SCE_GS_PSMCT24, 0, 0, STORE_IMAGE_W, STORE_IMAGE_H, SCRN_W );

  index = 0;

  for ( cnt = 0; cnt < STORE_IMAGE_H; cnt++ )
    {

      if ( ( ( frame_cnt == NULL ) && ( oddeven == NULL ) )
	   || ( ( frame_cnt ) && ( oddeven ) ) )
	{
	  sceWrite( fd, dst1 + index, STORE_IMAGE_W * 3 );
	  sceWrite( fd, dst2 + index, STORE_IMAGE_W * 3 );
	}
      else
	{
	  sceWrite( fd, dst2 + index, STORE_IMAGE_W * 3 );
	  sceWrite( fd, dst1 + index, STORE_IMAGE_W * 3 );
	}


      index += 120;		// (STORE_IMAGE_W * 3)/16;
      printf( "written line:%d\n", cnt );

    }

  free( dst1 );
  free( dst2 );

  sceClose( fd );


}				// end DumpImage

*/



//--------------------------------------------------------------------------------------------
// Function:    StoreTextureVIF1()
// Description: Stores texture data from GS to main memory or scratchpad memory.  See notes on
//              data flow
// Paramaters:  u_long128* base_addr: base address of stored texture data in MAIN/SPR memory
//              short start_addr: start address in GS memory to transfer from
//              short pixel_mode: pixel mode of stored data
//              short x: x location in GS memory
//              short y: y location in GS memory
//              short w: width of stored image
//              short h: height of stored image
//							short frame_width: width of frame buffer
// Returns:     N/A
// Notes: -     data flow of store image
//              	- wait for all 3 PATHS to GIF to be complete (FLUSHA vif code)
//              	- disable PATH3 transfer using MSKPATH3 vifcode (mask)
//              	- set BITBLTBUF register (source parameters)
//              	- set TRXPOS register (x,y postion)
//              	- set TRXREG register (width and height)
//              	- set FINISH register (set event)
//              	- set TRXDIR register (LOCAL->HOST)
//              	- get previous Interrupt Mask Control (IMR)
//              	- enable the FINISH event
//              	- send GIF packet to GS
//              	- wait for DMA to be complete
//              	- wait for FINISH event to be generated  (all drawing is complete)
//              	- change direction of the GS bus
//              	- change direction of VIF1-FIFO (VIF1_STAT.FDR)
//              	- DMA GS data to main/spr memory
//              	- wait for DMA to be complete
//              	- restore direction of the GS bus
//              	- restore direction of VIF1-FIFO 
//              	- restore previous Interrupt Mask Control (IMR)
//              	- enable the FINISH event
//              	- enable PATH3 transfer using MSKPATH3 vifcode
//--------------------------------------------------------------------------------------------

void StoreTextureVIF1( u_long128 * base_addr, short start_addr, short pixel_mode, short x,
		       short y, short w, short h, short frame_width )
{

  int texture_qwc;
  sceVif1Packet vif1_pkt;
  u_long128 settup_base[10];
  int buff_width;
  static u_int enable_path3[4] __attribute__ ( ( aligned( 16 ) ) ) = {
    0x06000000,
    0x00000000,
    0x00000000,
    0x00000000,
  };

  // get quad word count for image
  if ( pixel_mode == SCE_GS_PSMCT32 )
    texture_qwc = ( w * h * 32 ) >> 7;
  else if ( pixel_mode == SCE_GS_PSMCT24 )
    texture_qwc = ( w * h * 24 ) >> 7;
  else if ( pixel_mode == SCE_GS_PSMCT16 )
    texture_qwc = ( w * h * 16 ) >> 7;
  else if ( pixel_mode == SCE_GS_PSMT8 )
    texture_qwc = ( w * h * 8 ) >> 7;
  else
    texture_qwc = ( w * h * 4 ) >> 7;

  if ( texture_qwc > GIFTAG_MAX_NLOOP )
    {
      printf( "ERROR: Texture QWC is greater then GIFTAG_NLOOP! line:(%d), file:(%s)\n", __LINE__,
	       __FILE__ ); 
			exit( 0 );
    }

  buff_width = frame_width >> 6;

  if ( buff_width <= 0 )
    buff_width = 1;

  // set base address of GIF packet
  sceVif1PkInit( &vif1_pkt, &settup_base[0] );
  sceVif1PkReset( &vif1_pkt );

  // will start transfer with VIF code and GS data will follow
  sceVif1PkAddCode( &vif1_pkt, SCE_VIF1_SET_NOP( 0 ) );
  // disable PATH 3 transfer
  sceVif1PkAddCode( &vif1_pkt, SCE_VIF1_SET_MSKPATH3( 0x8000, 0 ) );
  // wait for all 3 PATHS to GS to be complete
  sceVif1PkAddCode( &vif1_pkt, SCE_VIF1_SET_FLUSHA( 0 ) );
  // transfer 6 QW's to GS
  sceVif1PkAddCode( &vif1_pkt, SCE_VIF1_SET_DIRECT( 6, 0 ) );

  // GIF tag for texture settings         
  sceVif1PkAddGsData( &vif1_pkt, SCE_GIF_SET_TAG( 5, GIFTAG_EOP, NULL, NULL, SCE_GIF_PACKED, 1 ) );
  sceVif1PkAddGsData( &vif1_pkt, 0xEL );

  // set transmission between buffers
  sceVif1PkAddGsData( &vif1_pkt, SCE_GS_SET_BITBLTBUF( start_addr, buff_width, pixel_mode,	// SRC
						       NULL, NULL, NULL ) );	// DEST
  sceVif1PkAddGsData( &vif1_pkt, SCE_GS_BITBLTBUF );

  // set transmission area between buffers        ( source x,y  dest x,y  and direction )
  sceVif1PkAddGsData( &vif1_pkt, SCE_GS_SET_TRXPOS( x, y, 0, 0, 0 ) );
  sceVif1PkAddGsData( &vif1_pkt, SCE_GS_TRXPOS );

  // set size of transmission area 
  sceVif1PkAddGsData( &vif1_pkt, SCE_GS_SET_TRXREG( w, h ) );
  sceVif1PkAddGsData( &vif1_pkt, SCE_GS_TRXREG );

  // set FINISH event occurrence request
  sceVif1PkAddGsData( &vif1_pkt, ( u_long ) ( 0x0 ) );
  sceVif1PkAddGsData( &vif1_pkt, SCE_GS_FINISH );

  // set transmission direction  ( LOCAL -> HOST Transmission )
  sceVif1PkAddGsData( &vif1_pkt, SCE_GS_SET_TRXDIR( 1 ) );
  sceVif1PkAddGsData( &vif1_pkt, SCE_GS_TRXDIR );

  // get packet size in quad words        
  sceVif1PkTerminate( &vif1_pkt );




  // get current IMR status
//  u_long prev_imr = 0;
//  prev_imr = sceGsPutIMR( sceGsGetIMR(  ) | 0x0200 );    // <<<<<  Mick, removed.


  // set the FINISH event
  DPUT_GS_CSR( GS_CSR_FINISH_M );



  // DMA from memory and start DMA transfer
  FlushCache( WRITEBACK_DCACHE );
  
	DPUT_D1_QWC( 0x7 );
  
	DPUT_D1_MADR( ( u_int ) vif1_pkt.pBase & DMA_MEM );

  
	DPUT_D1_CHCR( 1 | ( 1 << 8 ) );

  asm __volatile__( " sync.l " );


  // check if DMA is complete (STR=0)
  while ( DGET_D1_CHCR(  ) & 0x0100 );
//  printf( " 5 GS registers set\n" );

  // check if FINISH event occured
  while ( ( DGET_GS_CSR(  ) & GS_CSR_FINISH_M ) == 0 );
//  printf( " Finish event complete\n" );



  // change VIF1-FIFO transfer direction (VIF1 -> MAIN MEM or SPR)
  *VIF1_STAT = 0x00800000;


  // change GS bus direction (LOCAL->HOST)
  DPUT_GS_BUSDIR( ( u_long ) 0x00000001 );


//  printf( " Changed VIF1 and GS direction complete\n" );

  // DMA to memory and start DMA transfer
  FlushCache( WRITEBACK_DCACHE );
  
	DPUT_D1_QWC( texture_qwc );
  
	DPUT_D1_MADR( ( u_int ) base_addr & DMA_MEM );

  
	DPUT_D1_CHCR( 0 | ( 1 << 8 ) );
  
	asm __volatile__( " sync.l " );

  // check if DMA is complete (STR=0)
  while ( DGET_D1_CHCR(  ) & 0x0100 );
//  printf( " Transferred:(%d) QW from GS -> MEM complete\n", texture_qwc );



  // change VIF1-FIFO transfer direction (MAIN MEM or SPR -> VIF1)
  *VIF1_STAT = 0;

  // change GS bus direction (HOST->LOCAL)
  DPUT_GS_BUSDIR( ( u_long ) 0 );


  // restore previous IMR status
//  sceGsPutIMR( prev_imr );	 // <<<<<  Mick, removed.

  
  
  // set the FINISH event
  DPUT_GS_CSR( GS_CSR_FINISH_M );
//  printf( " Restore VIF1 and GS direction complete\n" );

  // MSKPATH3 is now enabled to allow transfer via PATH3
  DPUT_VIF1_FIFO( *( u_long128 * ) enable_path3 );

//  printf( " Restore PATH3 direction complete\n" );


}				// end StoreTextureVIF1



//----------------------------------------------EOF-------------------------------------------






/*****************************************************************************
**								  Externals									**
*****************************************************************************/


namespace NxPs2
{
	void	WaitForRendering();
}

namespace Gfx
{




/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

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

void	Manager::DumpVRAMUsage( void )
{
	printf ("ERROR: DumpVRAMUsage is no longer supported, due to dynamic textures\n");	
}


void 	save_to_pc(const char *fileroot,uint8* pixelBuffer)
{
		// Try to find a good filename of the format filebasexxx.bmp.  "Good" is
		// defined here as one that isn't already used.
		char fileName[ 132 ];
		int i = 0;
		while ( TRUE ) {
			sprintf( fileName, "Screens\\%s%03d.bmp", fileroot, i );
	
			// Found an unused one!  Yay!
			if ( FALSE == File::Exist( fileName ))
				break;
	
			i++;
		}
		printf ("Saving Screenshot %s\n",fileName);
		// Write out the file.	
		void *rwfd;
		rwfd = File::Open( fileName, "wb" );
		Dbg_MsgAssert(rwfd, ("Couldn't open %s for writing on the PC.", fileName));
		File::Write(pixelBuffer,1,640*448*3+HEADSIZE,rwfd);
		File::Close( rwfd );

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::ScreenShot( const char *fileroot )
{


	#if 1

	bool	compact = true;

	FlushCache( 0 );
	sceGsSyncPath( 0, 0 );

	NxPs2::WaitForRendering(); 		// This will also ensure the DMA buffers are free
	sceGsSyncPath( 0, 0 );



	uint8 *pixelBuffer;
	// Get a huge chunk of memory and dump the video buffer into it.
	pixelBuffer = NxPs2::dma::pRuntimeBuffer + HEADSIZE + 10;

	// for compact mode, we need 640*448*31/32*3 + 640*448/32*4  + 54 bytes = 833280 + 35840 + 54 = 869174 bytes

	int lines=32;			// lines per StoreImage (we need to do it in at last two)
	for (int i=0;i<448/lines;i++)
	{
		// not got that much memory, so we convert the buffer in place
		uint32 *p_pixel4 =  (uint32*)( pixelBuffer +i*640*3*lines );
		
		StoreTextureVIF1(
		(u_long128*)( p_pixel4 ),
		0,  // offset....
		SCE_GS_PSMCT32,
		0,
		i*lines,
		640,
		lines,
		640 );

		uint8 *p_pixel3 =  (uint8*) p_pixel4;
		uint8 r, g, b;
		for (int x = 0;x < 640*lines;x++)
		{
			// 
			r = ((( *p_pixel4 ) >> 16 ) & 0xff ) ;
			g = ((( *p_pixel4 ) >> 8 ) & 0xff ) ;
			b = ((( *p_pixel4++ )  ) & 0xff ) ;
			
			*p_pixel3++ = r;
			*p_pixel3++ = g;
			*p_pixel3++ = b;
		}
	}
	
	// flip the buffer upside down in place
	uint8*		top_line = pixelBuffer;
	uint8*		bot_line = pixelBuffer + 640*3*447;
	for (int line = 0;line <224; line++)
	{
		for (int b = 0;b<640*3;b++)
		{
			uint8 t = top_line[b];
			top_line[b] = bot_line[b];
			bot_line[b] = t;
		}
		top_line += 640*3;
		bot_line -= 640*3;
	}

	// and insert the header
	SetupBMPHeader((uint8*)NxPs2::dma::pRuntimeBuffer+10,640,448);

	// Finally, I'm going to copy the whole pile of crap down 10 bytes
	// in case anything that saves it relies on it being 16 byte aligned
	uint8 *p1 = (uint8*)NxPs2::dma::pRuntimeBuffer+10;
	uint8 *p2 = (uint8*)NxPs2::dma::pRuntimeBuffer;
	for (int i=0;i<640*448*3+HEADSIZE;i++)
	{
		*p2++ = *p1++;
	}  
	pixelBuffer = (uint8*)NxPs2::dma::pRuntimeBuffer; 

	if (Script::GetInt("memcard_screenshots"))
	{
		printf ("STUBBED!!! Saving Screenshot to TH4MC???\n");
//		CFuncs::SaveDataFile("TH4MC", NxPs2::dma::pRuntimeBuffer, 640*448*3+HEADSIZE);
	}
	else
	{
		save_to_pc(fileroot,pixelBuffer);	
	}
	
	if (!compact)
	{
		// Release the memory for the video buffer.
		delete( pixelBuffer );
	}	
	
	#else
		printf ("Screenshot functionality stubbed out\n");
	
	#endif

	
    FlushCache(0);


}

// Called byte dumpshots
void Manager::DumpMemcardScreeenshots()
{
/*
	char name[100];
	for (int i=0;i<12;i++)
	{
		sprintf(name,"TH4MC%03d",i);
		printf ("Checking for %s \n",name);
		if (CFuncs::LoadDataFile(name, NxPs2::dma::pRuntimeBuffer, 640*448*3+HEADSIZE))
		{
			save_to_pc("MemCard", NxPs2::dma::pRuntimeBuffer);
		}
		else
		{
			printf ("not there\n");
		}
	}
*/	
	printf ("DumpMemcardScreeenshots STUBBED\n");
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/





/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Gfx
