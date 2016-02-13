////////////////////////////////////////////////////////////////////////////
//
// Feeler.cpp	  - Mick
//
// A feeler is a 3D line segment that can check for collision with
// the world, and return information about the faces it collides with
//
// it's derived from the line type
// so operations you can do with a line
// can also be done with the feeler		  
// (See geometry.cpp for line operations)


#include <engine/feeler.h>

#include <core/defines.h>
#include <core/singleton.h>
#include <core/math.h>
#include <gfx/debuggfx.h>
#include <core/math/geometry.h>

#include <gel/collision/collision.h>

#include <gel/object/compositeobject.h>
#include <gel/modman.h>

#include <gfx/nx.h>

#define PRINT_TIMES 0
						
Nx::CCollCache *		CFeeler::sp_default_cache = NULL;

					
CFeeler::CFeeler()
{
	init();
}

CFeeler::CFeeler(const Mth::Vector &start, const Mth::Vector &end)
{
	m_start = start;
	m_end = end;
	m_col_data.surface.normal.Set(1.0, 1.0, 1.0);
	init();
}

void CFeeler::init (   )
{
	// When we create a feeler
	// we use the default "ignore" settings
	// as they will be used the most
	m_col_data.ignore_1 = mFD_NON_COLLIDABLE; 		// ignore NON_COLLIDABLES
	m_col_data.ignore_0 = 0;						// don't ignore anything else
	mp_callback = NULL;
	mp_cache = NULL;

	// Now un-initialized...
	m_col_data.surface.normal.Set();
	m_col_data.surface.point.Set();
}

void CFeeler::SetIgnore(uint16 ignore_1, uint16 ignore_0)
{
	m_col_data.ignore_1 = ignore_1;
	m_col_data.ignore_0 = ignore_0;
}


void CFeeler::SetStart(const Mth::Vector &start)
{
	m_start = start;
}

void CFeeler::SetEnd(const Mth::Vector &end)
{
	m_end = end;
}

void CFeeler::SetLine(const Mth::Vector &start, const Mth::Vector &end)
{
	SetStart(start);
	SetEnd(end);
}

							  

// This callback gets called by the CLd:: collision code
// (which was in turn a callback called from elsewhere)
// and then this calls my Callback
// we should try to simplyify this chain of callbacks
// whilst still keeping platform and engine independence
static void s_feeler_collide_callback(Nx::CollData* p_col_data)
{
	CFeeler *p_feeler = (CFeeler*) p_col_data->p_callback_data;
	p_feeler->mp_callback(p_feeler); 
}

// This is the guts, the interface to the lower level collision code
bool	CFeeler::GetCollision(bool movables, bool bfar)
{
#if PRINT_TIMES
	static uint64 s_total_time = 0, s_num_collisions = 0;
	uint64 start_time = Tmr::GetTimeInUSeconds();
#endif
	
 #ifdef	__PLAT_NGPS__		
	//	snProfSetRange( -1, (void*)0, (void*)-1);
	//	snProfSetFlagValue(0x01);
 #endif
   
	
	Mth::Line is;

	// just copy over the start and the end of the line directly.
	is.m_start = m_start;
	is.m_end = m_end;

	m_col_data.p_callback_data = this;	// callback data is this current feeler

	// initialize collision info
	m_col_data.coll_found = false;
	m_col_data.trigger = false;
	m_col_data.script = 0;				// This is a checksum

	m_col_data.mp_callback_object = NULL;
						  
	void *callback = 0;					
	if (mp_callback)
	{
	   	callback = s_feeler_collide_callback;
	}

	Nx::CCollCache *p_cache = (mp_cache) ? mp_cache : sp_default_cache;

	bool	collision;
	mp_movable_collision_obj = NULL;
	m_movable_collision_id = 0;
	if (bfar)
	{
		collision = Nx::CCollObj::sFindFarStaticCollision( is, &m_col_data, callback, p_cache );
	}
	else
	{
		collision = Nx::CCollObj::sFindNearestStaticCollision( is, &m_col_data, callback, p_cache );
	}

	// Now see if we can find a closer movable
	if (movables)
	{
		if (bfar)
		{
			mp_movable_collision_obj = Nx::CCollObj::sFindFarMovableCollision( is, &m_col_data, callback, p_cache );
		}
		else
		{
			mp_movable_collision_obj = Nx::CCollObj::sFindNearestMovableCollision( is, &m_col_data, callback, p_cache );
		}
		if (mp_movable_collision_obj)
		{
			m_movable_collision_id = mp_movable_collision_obj->GetID();
		}
	}

	m_dist = m_col_data.dist;
	m_point = m_col_data.surface.point;
	m_normal = m_col_data.surface.normal;

#if 0
	if (collision)
	{
		Gfx::AddDebugLine(m_point,m_point + (m_normal * 100.0f),coll_color,10);
	}

	//static int num_print = 1;
	if (/*collision && (num_print++ % 100) ==*/ 0)
	{
		Dbg_Message("\nStart point (%f, %f, %f)", m_start[X], m_start[Y], m_start[Z]);
		Dbg_Message("End point (%f, %f, %f)", m_end[X], m_end[Y], m_end[Z]);

		Dbg_Message("Collision Distance %f", m_dist);
		Dbg_Message("Collision point (%f, %f, %f)", m_point[X], m_point[Y], m_point[Z]);
		Dbg_Message("Collision normal (%f, %f, %f)", m_normal[X], m_normal[Y], m_normal[Z]);
	}
#endif


 #ifdef	__PLAT_NGPS__		
	//	snProfSetRange( 4, (void*)NULL, (void*)-1);
 #endif		

#if PRINT_TIMES
	uint64 end_time = Tmr::GetTimeInUSeconds();
	s_total_time += end_time - start_time;

	if (++s_num_collisions >= 1000)
	{
		Dbg_Message("Feeler time %d us", s_total_time);
		s_total_time = s_num_collisions = 0;
	}
#endif

	
	return collision || (mp_movable_collision_obj != NULL);



}

//
// Just checks the movable collisions
//
bool CFeeler::GetMovableCollision(bool bfar)
{
	Mth::Line is;

	// just copy over the start and the end of the line directly.
	is.m_start = m_start;
	is.m_end = m_end;

////////////////////////////////////////////////////////////
// Clear data than might have been set on a previous call
// (change suggested by Andre at LF, 4/28/03)
	m_col_data.coll_found = false;
	m_col_data.trigger = false;
	m_col_data.script = 0;				// This is a checksum
	m_col_data.mp_callback_object = NULL;
// End of change
////////////////////////////////////////////////////////////


	m_col_data.p_callback_data = this;	// callback data is this current feeler
						  
	void *callback = 0;					
	if (mp_callback)
	{
	   	callback = s_feeler_collide_callback;
	}

	Nx::CCollCache *p_cache = (mp_cache) ? mp_cache : sp_default_cache;

	if (bfar)
	{
		mp_movable_collision_obj = Nx::CCollObj::sFindFarMovableCollision( is, &m_col_data, callback, p_cache );
	}
	else
	{
		mp_movable_collision_obj = Nx::CCollObj::sFindNearestMovableCollision( is, &m_col_data, callback, p_cache );
	}
	
	if (mp_movable_collision_obj)
	{
		m_movable_collision_id = mp_movable_collision_obj->GetID();
	}

	m_dist = m_col_data.dist;
	m_point = m_col_data.surface.point;
	m_normal = m_col_data.surface.normal;

	return mp_movable_collision_obj != NULL;
}

// Check to see that if we have collision, that we have enough data to
// calculate the brightness
bool	CFeeler::IsBrightnessAvailable()
{
	if (m_col_data.coll_found)
	{
		Dbg_MsgAssert(m_col_data.p_coll_object, ("IsBrightnessAvailable(): collision w/o a collsion object"));
		return m_col_data.p_coll_object->IsTriangleCollision();
	}

	return true;
}

// get the brightness 0.0 .. 1.0 of the point of the last
// collision, based on the RGB values of the triangle, interpolatd to
// the collision point
float	CFeeler::GetBrightness()
{
	float brightness;
	
	struct SGPoint
	{
		Mth::Vector	p;
		float		v;			// Brightness in range [0.0, 1.0].
	};

	uint32	i;
	SGPoint	point[3];

	
	// If no collision found, set default brightness.
	if( !m_col_data.coll_found )
	{
		brightness = 0.5f;
	}
	else
	{
		Nx::CCollObj*				p_object	= m_col_data.p_coll_object;
		const Nx::SCollSurface*		p_face		= &m_col_data.surface;
		uint32						face_index	= p_face->index;
		//Cld::CCollSector::SFace*	p_polygon	= p_object->mp_faces + face_index;

		// Extract contact point.
		float x = p_face->point[X];
		float y = p_face->point[Y];
		float z = p_face->point[Z];

		Dbg_MsgAssert(p_object->IsTriangleCollision(),("Not triangle collision !!!"));

		Nx::CCollObjTriData *p_tri_data=p_object->GetGeometry();
		Dbg_MsgAssert(p_tri_data,("NULL p_tri_data"));

		// Get all the vertices into a nice simple array of (x,y,z,v).
		for( i = 0; i < 3; ++i )
		{
			//uint16 *vert_idxs = p_object->GetFaceVertIndicies(face_index);

			const Mth::Vector & vertex = p_object->GetVertexPos(p_tri_data->GetFaceVertIndex(face_index,i));

			// Check that the vertices match.
			point[i].p.Set( vertex.GetX() - x, vertex.GetY() - y, vertex.GetZ() - z );

			// Here is where the conversion is made from values [0,255] to [0,1].
#ifdef __PLAT_NGC__
			point[i].v	= (float)( p_tri_data->GetVertexIntensity(face_index, i)) * ( 1.0f / 255.0f );
#else
			point[i].v	= (float)( p_tri_data->GetVertexIntensity(p_tri_data->GetFaceVertIndex(face_index, i)) ) * ( 1.0f / 255.0f );
#endif		// __PLAT_NGC__
		}

		// Calculate the intersection of the line from Point 0 through the origin, with the line 1-2.
		float		mua, mub;
		Mth::Line	line1( point[0].p, Mth::Vector( 0.0f, 0.0f, 0.0f ));
		Mth::Line	line2( point[1].p, point[2].p );
		Mth::Vector	a, b;

		Mth::LineLineIntersect( line1, line2, &a, &b, &mua, &mub, false );

		float l0, l1;
		Mth::Vector length;

		// First interpolation a = i( 1, 2 ).
		length				= a - point[1].p;
		l0					= length.Length();
		length				= point[2].p - point[1].p;
		l1					= length.Length();
		float va			= point[1].v + (( point[2].v - point[1].v ) * ( l0 / l1 ));

		// Second interpolation b = i( 0, a ).
		l0					= point[0].p.Length();
		length				= a - point[0].p;
		l1					= length.Length();
		brightness	= point[0].v + (( va - point[0].v ) * ( l0 / l1 ));
	}
	
#	ifdef __PLAT_XBOX__
	// Xbox vertex colors run on a different scale to Ps2.
	brightness *= 0.5f;
#	endif
	
	return brightness;
}


bool CFeeler::GetCollision(const Mth::Vector &start, const Mth::Vector &end, bool movables)
{
	SetLine(start,end);
	return GetCollision(movables);
}

bool CFeeler::GetMovableCollision(const Mth::Vector &start, const Mth::Vector &end)
{
	SetLine(start,end);
	return GetMovableCollision();
}

bool	CFeeler::IsMovableCollision()
{
	if (mp_movable_collision_obj.Convert())
	{
		return true;
	}

// if it was movable, and now we have no movable object
// then kill the id and the node name
// so we don't get stung by the snap to ground assuming they are still valid....
	if (m_movable_collision_id)
	{
		m_movable_collision_id = 0;
		m_col_data.node_name = 0;		// we are SO like not colliding with anything!
	}
	return false;
}

Obj::CCompositeObject *		CFeeler::GetMovingObject()	// { return mp_movable_collision_obj;}
{
	return mp_movable_collision_obj.Convert();
}

// if we are in the middle of a callback, then
// we need to look at the callback object													   
Obj::CCompositeObject *		CFeeler::GetCallbackObject() const
{
	return m_col_data.mp_callback_object;
}


void CFeeler::DebugLine(int r, int g, int b, int num_frames)
{	
	Gfx::AddDebugLine( m_start, m_end, MAKE_RGB( r, g, b ),MAKE_RGB( r,g, b ), num_frames );
}


