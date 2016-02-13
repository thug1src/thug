#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <dolphin.h>
#include "p_frame.h"

class NsCamera
{
	friend class NsMatrix;

	NsMatrix			m_matrix;
    NsVector			m_up;
    NsVector			m_camera;
	NsMatrix			m_current;

	NsMatrix			m_view;

	NsFrame			  * m_pFrame;

	float				viewx, viewy;
	float				view1x, view1y;

	// Previously in viewport.
	GXProjectionType	m_type;
	Mtx44				m_viewmatrix;
	u32					m_clip_x, m_clip_y;
	u32					m_clip_w, m_clip_h;

	void		_setCurrent ( NsMatrix * object );
public:
				NsCamera		();
				~NsCamera		();

	void		pos				( float x, float y, float z );
	void		up				( float x, float y, float z );
	void		lookAt			( float x, float y, float z );
	void		begin			( void );
	void		end				( void );

	void		offset			( float x, float y, float z );

	NsMatrix  * getCurrent		( void ) { return &m_current; };
	NsMatrix  * getViewMatrix	( void );

	NsFrame	  * getFrame		( void ) { return m_pFrame; };
	void		setFrame		( NsFrame * p ) { m_pFrame = p; };

	// Previously in viewport.
	void		perspective		( void ) { m_type = GX_PERSPECTIVE; };
	void		perspective		( u32 x, u32 y, u32 width, u32 height );
	void		perspective		( float width, float height, float znear, float zfar );
	void		perspective		( float fovy, float aspect );
	void		orthographic	( void ) { m_type = GX_ORTHOGRAPHIC; };
	void		orthographic	( u32 x, u32 y, u32 width, u32 height );

	void		setViewWindow	( float x, float y );

	void		clip			( u32 x, u32 y, u32 width, u32 height );

	float		GetViewMatrix	( int x, int y ) { return m_viewmatrix[x][y]; }
	void		SetViewMatrix	( int x, int y, float v ) { m_viewmatrix[x][y] = v; }
};

#endif		// _CAMERA_H_
