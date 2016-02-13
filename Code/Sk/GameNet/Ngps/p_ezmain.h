/*  
//  ------------------------------------------------------------------------

File: main.h

headers for EE library components.

//  ------------------------------------------------------------------------
*/

#ifndef _MAIN_H
#define _MAIN_H

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <eekernel.h>
#include <libcdvd.h>
#include <libmc.h>
#include <libpad.h>
#include <sif.h>
#include <sifdev.h>
#include <sifcmd.h>
#include <sifrpc.h>

#include <libnet.h>


// <sce/common/include/libver.h> only present after 2.4.0
// manually define SCE_LIBRARY_VERSION if necessary.
#if (0)
#define SCE_LIBRARY_VERSION     0x2340
#else
#include <libver.h>
#endif

#include "p_netcnfif.h"

#include "p_ezcommon.h"
#include "p_ezconfig.h"
//#include "util.h"
#include "p_libezcnf.h"
//#include "libezctl.h"
//#include "trc_hdd.h"


#endif  // _MAIN_H
