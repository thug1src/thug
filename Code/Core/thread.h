/*******************************************************************
*
*    DESCRIPTION: thread.h
*
*    AUTHOR:	JAB
*
*    HISTORY:	
*
*    DATE:9/1/2000
*
*******************************************************************/

/** include files **/
#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
	  
#ifdef __PLAT_WN32__
#else
#	ifdef __PLAT_XBOX__
#	else
#		ifdef __PLAT_NGPS__
#			include "thread/ngps/t_thread.h"
#		elif defined( __PLAT_NGC__ )
#			include "thread/ngc/t_thread.h"
#		else
#			error Unsupported Platform
#		endif
#	endif
#endif


