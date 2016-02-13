#ifndef _DISP_H_
#define _DISP_H_

#include <libgraph.h>

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/macros.h>
#include <core/singleton.h>

namespace Flx
{



// ////////////////////////////////////////////////////////////////
//
// Functions
//
void clearGsMem(int r, int g, int b, int disp_width, int disp_height);
void setImageTag(u_int *tags, void *image, int index,
		 int image_w, int image_h);
void startDisplay(int waitEven);
void endDisplay();

} // namespace Flx

#endif _DISP_H_
