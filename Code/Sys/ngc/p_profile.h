#ifndef _PROFILE_H_
#define _PROFILE_H_

#include <dolphin.h>
#include "p_prim.h"

class NsProfile
{
	OSStopwatch		m_watch;
	OSTime			m_latch;
	OSTime			m_accumulated;
	float		  *	m_pHistoryBuffer;
	unsigned int	m_historyBufferSize;
	unsigned int	m_historyEntry;
public:
			NsProfile	();
			NsProfile	( char * pName );
			NsProfile	( char * pName, unsigned int historySize );

	void	start		( void );
	void	stop		( void );
	void	draw		( float x0, float y0, float x1, float y1, GXColor color );
	void	histogram	( float x0, float y0, float x1, float y1, GXColor color );
	void	append		( GXColor color, bool update );
};

#endif		// _PROFILE_H_













