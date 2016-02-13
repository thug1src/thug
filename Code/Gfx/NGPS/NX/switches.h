#ifndef __SWITCHES_H
#define __SWITCHES_H

#ifdef __PLAT_NGPS__
#include <sys/config/config.h>
#endif // __PLAT_NGPS__

#define	malloc(x) error Old Memory Function
#define	calloc(x) error Old Memory Function
#define	realloc(x) error Old Memory Function
#define	free(x) error Old Memory Function


namespace NxPs2
{

#define USE_INTERRUPTS 1
#define	GIF_INTERRUPT  1
#define VIF1_INTERRUPT 0
#define GS_INTERRUPT   1

#define TIMING_BARS 0

#define C32 1
#define C16 0
#define Z24 1
#define Z16 0

#ifdef __PLAT_NGPS__

#define HRES_NTSC	(640)
#define VRES_NTSC	(448)
#define HRES_PAL	(512)
#define VRES_PAL	(512)

#define HRES (Config::NTSC() ? HRES_NTSC : HRES_PAL)
#define VRES (Config::NTSC() ? VRES_NTSC : VRES_PAL)

#define XOFFSET ((2048 - (HRES >> 1)) << 4)
#define YOFFSET ((2048 - (VRES >> 1)) << 4)

// Start and size of buffers in 2K Word size blocks
#define FRAME_START		(0)
#define FRAME_SIZE		(((HRES * VRES) * 4) / (2048 * 4))		// 32-bit frame buffer
#define DISPLAY_START	(FRAME_START + FRAME_SIZE)
#define DISPLAY_SIZE	(((HRES * VRES) * 2) / (2048 * 4))		// 16-bit display buffer
#define ZBUFFER_START	(DISPLAY_START + DISPLAY_SIZE)
#define ZBUFFER_SIZE	(((HRES * VRES) * 4) / (2048 * 4))		// 32-bit aligned ZBuffer

#endif // __PLAT_NGPS__

#define FTOI_TRICK 1

#define STENCIL_SHADOW 0
#define OLD_FOG 0

} // namespace NxPs2

#endif // __SWITCHES_H

