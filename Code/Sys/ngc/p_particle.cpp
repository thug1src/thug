///*****************************************************************************
//**																			**
//**			              Neversoft Entertainment							**
//**																		   	**
//**				   Copyright (C) 2000 - All Rights Reserved				   	**
//**																			**
//******************************************************************************
//**																			**
//**	Project:		Skate3         											**
//**																			**
//**	Module:			GFX            	 										**
//**																			**
//**	File name:		p_particle.cpp 											**
//**																			**
//**	Created:		08/16/2001	dc     										**
//**																			**
//**	Description:	NGC particle system interface							**
//**																			**
//*****************************************************************************/
//
//
///*****************************************************************************
//**							  	  Includes									**
//*****************************************************************************/
// 
//#include <sys/ngc/p_vector.h>
//#include "p_particle.h"
//#include "p_prim.h"
//
//
//
///*****************************************************************************
//**							  DBG Information								**
//*****************************************************************************/
//
//namespace Particle
//{
//
///*****************************************************************************
//**								  Externals									**
//*****************************************************************************/
//
//
///*****************************************************************************
//**								   Defines									**
//*****************************************************************************/
//
//#define fmin2f( a, b )	((( a ) < ( b )) ? ( a ) : ( b ))
//
///*****************************************************************************
//**								Private Types								**
//*****************************************************************************/
//
///*****************************************************************************
//**								 Private Data								**
//*****************************************************************************/
//
//static NsVector		screen_right;
//static NsVector		screen_up;
//static NsCamera*	p_current_cam	= NULL;
//
//
//
///*****************************************************************************
//**								 Public Data								**
//*****************************************************************************/
//
///*****************************************************************************
//**							  Private Prototypes							**
//*****************************************************************************/
//
///*****************************************************************************
//**							   Private Functions							**
//*****************************************************************************/
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::seedParticle( int p, float t_max )
//{
//	Dbg_Assert(( t_max >= 0.0f ) && ( t_max <= 1.0f ));
//
//	parametric_particle* p_part = mp_particles + p;
//
//	// Figure current parametric parameter.
//	p_part->t = t_max;
//
//	// Figure intial velocity.
//	float angle_range_x	= (((float)rand() / RAND_MAX ) * 2.0f ) - 1.0f;
//	float angle_range_y	= (((float)rand() / RAND_MAX ) * 2.0f ) - 1.0f;
//
//	NsVector vel;
//	vel.x				= m_emitter_angle * angle_range_x;
//	vel.y				= m_emitter_angle * angle_range_y;
//	vel.z				= 1.0f - (( vel.x * vel.x ) + ( vel.y * vel.y ));
//
//	float speed			= m_min_speed - ( m_variation_speed * 0.5f ) + ( m_variation_speed * ((float)rand() / RAND_MAX ));
//	speed			   *= 1.0f - fmin2f(( angle_range_x * angle_range_x ) + ( angle_range_y * angle_range_y ), -1.0f * m_damping );
//	p_part->initial_velocity.scale( vel, speed );
//
//	// Figure initial position.
//	p_part->start_pos.x			= -m_emitter_width + ( 2.0f * m_emitter_width * ((float)rand() / RAND_MAX ));
//	p_part->start_pos.y			= -m_emitter_height + ( 2.0f * m_emitter_height * ((float)rand() / RAND_MAX ));
//	p_part->start_pos.z			= 0.0f;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//bool NsParticleAtomic::cull( void )
//{
//	int				lp;
//	unsigned int	code;
//	unsigned int	codeAND;
//	f32				rx[8], ry[8], rz[8];
//	f32				p[GX_PROJECTION_SZ];
//	f32				vp[GX_VIEWPORT_SZ];
//	u32				clip_x;
//	u32				clip_y;
//	u32				clip_w;
//	u32				clip_h;
//	float			clip_l;
//	float			clip_t;
//	float			clip_r;
//	float			clip_b;
//	MtxPtr			view;
//	float			minx, miny, minz;
//	float			maxx, maxy, maxz;
//
//	GXGetProjectionv( p );
//	GXGetViewportv( vp );
//	GXGetScissor( &clip_x, &clip_y, &clip_w, &clip_h );
//	clip_l = (float)clip_x;
//	clip_t = (float)clip_y;
//	clip_r = (float)( clip_x + clip_w );
//	clip_b = (float)( clip_y + clip_h );
//
//	view = (MtxPtr)( p_current_cam->getCurrent()->getRight());
//
//	NsMatrix* p_matrix = m_frame.getModelMatrix();
//
//	minx = m_bbox.m_min.x + p_matrix->getPosX();
//	miny = m_bbox.m_min.y + p_matrix->getPosY();
//	minz = m_bbox.m_min.z + p_matrix->getPosZ();
//	maxx = m_bbox.m_max.x + p_matrix->getPosX();
//	maxy = m_bbox.m_max.y + p_matrix->getPosY();
//	maxz = m_bbox.m_max.z + p_matrix->getPosZ();
//
//	GXProject ( minx, miny, minz, view, p, vp, &rx[0], &ry[0], &rz[0] );
//	GXProject ( minx, maxy, minz, view, p, vp, &rx[1], &ry[1], &rz[1] );
//	GXProject ( maxx, miny, minz, view, p, vp, &rx[2], &ry[2], &rz[2] );
//	GXProject ( maxx, maxy, minz, view, p, vp, &rx[3], &ry[3], &rz[3] );
//	GXProject ( minx, miny, maxz, view, p, vp, &rx[4], &ry[4], &rz[4] );
//	GXProject ( minx, maxy, maxz, view, p, vp, &rx[5], &ry[5], &rz[5] );
//	GXProject ( maxx, miny, maxz, view, p, vp, &rx[6], &ry[6], &rz[6] );
//	GXProject ( maxx, maxy, maxz, view, p, vp, &rx[7], &ry[7], &rz[7] );
//	bool visible = true;
//
//	// Generate clip code. {page 178, Procedural Elements for Computer Graphics}
//	// 1001|1000|1010
//	//     |    |
//	// ----+----+----
//	// 0001|0000|0010
//	//     |    |
//	// ----+----+----
//	// 0101|0100|0110
//	//     |    |
//	//
//	// Addition: Bit 4 is used for z behind.
//
//	codeAND	= 0x001f;
//
//	for ( lp = 0; lp < 8; lp++ ) {
//		// Only check x/y if z is valid (if z is invalid, the x/y values will be garbage).
//		if ( rz[lp] > 1.0f   ) {
//			code = (1<<4);
//		} else {
//			code = 0;
//			if ( rx[lp] < clip_l ) code |= (1<<0);
//			if ( rx[lp] > clip_r ) code |= (1<<1);
//			if ( ry[lp] > clip_b ) code |= (1<<2);
//			if ( ry[lp] < clip_t ) code |= (1<<3);
//		}
//		codeAND	&= code;
//	}
//
//	// If any bits are set in the AND code, the object is invisible.
//	if ( codeAND ) {
//		visible = false;
//	}
//	
//	return !visible;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::figureBoundingBox( void )
//{
//	// Cheesy, but it works reasonably well. Pass 1 particle through the system n times, sampling it's
//	// position at various intervals.
//	NsMatrix*					p_matrix	= m_frame.getModelMatrix();
//	struct parametric_particle*	p_part		= mp_particles;
//
//	m_bbox.m_min.x = m_bbox.m_max.x = 0.0f;
//	m_bbox.m_min.y = m_bbox.m_max.y = 0.0f;
//	m_bbox.m_min.z = m_bbox.m_max.z = 0.0f;
//
//	for( int pass = 0; pass < 16; ++pass )
//	{
//		for( ;; )
//		{
//			p_part->t += 0.11945f;
//
//			// First deal with reseeding any particles that have exceeded their lifetime.
//			if( p_part->t > 1.0f )
//			{
//				seedParticle( 0, p_part->t - 1.0f );
//				break;
//			}
//
//			// Now figure current position.
//			float time				= p_part->t * m_flight_time;
//
//			// The 0.5 multiply is already done when the force is set.
//			float half_time_squared	= time * time;
//			NsVector	pos, ut, at, temp;
//
//			ut.scale( *p_matrix->getRight(), p_part->initial_velocity.x * time );
//			temp.scale( *p_matrix->getUp(), p_part->initial_velocity.y * time );
//			ut.add( temp );
//			temp.scale( *p_matrix->getAt(), p_part->initial_velocity.z * time );
//			ut.add( temp );
//
//			at.scale( *p_matrix->getRight(), m_force.x * half_time_squared );
//			temp.scale( *p_matrix->getUp(), m_force.y * half_time_squared );
//			at.add( temp );
//			temp.scale( *p_matrix->getAt(), m_force.z * half_time_squared );
//			at.add( temp );
//
//			pos.set( ut.x + at.x, ut.y + at.y, ut.z + at.z );
//
//			if( pos.x < m_bbox.m_min.x )
//				m_bbox.m_min.x	= pos.x;
//			else if( pos.x > m_bbox.m_max.x )
//				m_bbox.m_max.x	= pos.x;
//
//			if( pos.y < m_bbox.m_min.y )
//				m_bbox.m_min.y	= pos.y;
//			else if( pos.y > m_bbox.m_max.y )
//				m_bbox.m_max.y	= pos.y;
//
//			if( pos.z < m_bbox.m_min.z )
//				m_bbox.m_min.z	= pos.z;
//			else if( pos.z > m_bbox.m_max.z )
//				m_bbox.m_max.z	= pos.z;
//
//		}
//	}
//
//	m_flags &= ~PARTICLE_FLAG_BBOX_DIRTY;
//}
//
//
//
///*****************************************************************************
//**							   Public Functions								**
//*****************************************************************************/
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void SetParticleAtomicCamera( NsCamera* p_cam )
//{
//	// Used to figure the right and up vectors for creating screen-aligned particle quads.
//	NsFrame*	p_frame		= p_cam->getFrame();
//	NsMatrix*	p_matrix	= p_frame->getModelMatrix();
//
//	NsVector	up( 0.0f, 1.0f, 0.0f );
//
//	// Get the 'right' vector as the cross product of camera 'at and world 'up'.
//	screen_right.cross( *p_matrix->getAt(), up );
//	screen_up.cross( screen_right, *p_matrix->getAt());
//
//	screen_right.normalize();
//	screen_up.normalize();
//
//	p_current_cam	= p_cam;
//}
//
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//NsParticleAtomic::NsParticleAtomic( int num_particles, NsMaterial* p_material )
//{
//	m_visible				= true;
//	m_num_particles			= num_particles;
//	m_initial_num_particles	= num_particles;
//
//	mp_particles = new struct parametric_particle[num_particles];
//
//	for( int i = 0; i < num_particles; ++i )
//	{
//		seedParticle( i, (float)rand() / (float)RAND_MAX );
//	}
//
//	// Flag to indicate the bounding box needs calculating.
//	m_flags |= PARTICLE_FLAG_BBOX_DIRTY;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//NsParticleAtomic::~NsParticleAtomic()
//{
//	if( mp_material )
//	{
//		delete mp_material;
//	}
//
//	delete [] mp_particles;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::SetNumParticles( int num )
//{
//	Dbg_Assert( num <= m_initial_num_particles );
//
//	// This value can only be set to less than it was initially.
//	if( num <= m_initial_num_particles )
//	{
//		m_num_particles = num;
//	}
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::update( float delta_t )
//{
//	// If time since last draw is increasing, probably not being drawn currently, so
//	// pointless letting it get beyond 1.0f.
//	m_time_since_last_draw += delta_t;
//
//	if( m_time_since_last_draw > m_flight_time )
//	{
//		m_time_since_last_draw -= m_flight_time;
//	}
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::draw( void )
//{
//	if( m_visible )
//	{
//		// Reseed particles if dirty.
//		if( m_flags & PARTICLE_FLAG_DIRTY )
//		{
//			for( int i = 0; i < m_initial_num_particles; ++i )
//			{
//				seedParticle( i, (float)rand() / (float)RAND_MAX );
//			}
//			m_flags &= ~PARTICLE_FLAG_DIRTY;
//		}
//
//		// Refigure bounding box if required.
//		if( m_flags & PARTICLE_FLAG_BBOX_DIRTY )
//		{
//			figureBoundingBox();
//		}
//
//		// Test bounding box against view frustum.
//		if( cull() )
//		{
//			return;
//		}
//
//		// Figure time since last draw as a ratio of flight time.
//		float t_delta = m_time_since_last_draw * m_inv_flight_time;
//
//		NsRender::begin();
//		NsRender::setZMode( NsZMode_LessEqual, 1, 0, 0 );
//
//	    // Set current vertex descriptor to enable position and color0. Both use 8b index to access their data arrays.
//		GXClearVtxDesc();
//		GXSetVtxDesc( GX_VA_POS, GX_DIRECT );
//	    GXSetVtxDesc( GX_VA_TEX0, GX_DIRECT );
//		GX::SetChanCtrl( GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE );
//
//		// Upload texture if present.
//		NsTexture* p_texture = ( mp_material ) ? ( mp_material->getTexture() ) : NULL;
//		if( p_texture )
//		{
//			p_texture->upload( NsTexture_Wrap_Clamp );
//		}
//
//		NsMatrix*	p_matrix	= m_frame.getModelMatrix();
//		Vec			mat_pos		= (Vec){ p_matrix->getPosX(), p_matrix->getPosY(), p_matrix->getPosZ()};
//
//		for( int i = 0; i < m_num_particles; ++i )
//		{
//			struct parametric_particle* p_part = mp_particles + i;
//
//			// First deal with reseeding any particles that have exceeded their lifetime.
//			if(( p_part->t += t_delta ) > 1.0f )
//			{
//				seedParticle( i, p_part->t - 1.0f );
//			}
//
//			Dbg_Assert( p_part->t <= 1.0f );
//
//			// Now figure current position.
//			float time				= p_part->t * m_flight_time;
//
//			// The 0.5 multiply is already done when the force is set.
//			float half_time_squared	= time * time;
//			NsVector	pos;
//			NsVector	p, ut, at, temp;
//
//			Vec*		p_mat_right = (Vec*)p_matrix->getRight();
//			Vec*		p_mat_up	= (Vec*)p_matrix->getUp();
//			Vec*		p_mat_at	= (Vec*)p_matrix->getAt();
//
//			p.scale( *p_matrix->getRight(), p_part->start_pos.x );
//			temp.scale( *p_matrix->getUp(), p_part->start_pos.y );
//			p.add( temp );
//			VECScale( p_mat_right, (Vec*)&p, p_part->start_pos.x );
//			VECScale( p_mat_up, (Vec*)&temp, p_part->start_pos.y );
//			VECAdd( (Vec*)&p, (Vec*)&temp, (Vec*)&p );
//
//			ut.scale( *p_matrix->getRight(), p_part->initial_velocity.x * time );
//			temp.scale( *p_matrix->getUp(), p_part->initial_velocity.y * time );
//			ut.add( temp );
//			temp.scale( *p_matrix->getAt(), p_part->initial_velocity.z * time );
//			ut.add( temp );
//			VECScale( p_mat_right, (Vec*)&ut, p_part->initial_velocity.x * time );
//			VECScale( p_mat_up, (Vec*)&temp, p_part->initial_velocity.y * time );
//			VECAdd( (Vec*)&ut, (Vec*)&temp, (Vec*)&ut );
//			VECScale( p_mat_at, (Vec*)&temp, p_part->initial_velocity.z * time );
//			VECAdd( (Vec*)&ut, (Vec*)&temp, (Vec*)&ut );
//
//			at.scale( *p_matrix->getRight(), m_force.x * half_time_squared );
//			temp.scale( *p_matrix->getUp(), m_force.y * half_time_squared );
//			at.add( temp );
//			temp.scale( *p_matrix->getAt(), m_force.z * half_time_squared );
//			at.add( temp );
//			VECScale( p_mat_right, (Vec*)&at, m_force.x * half_time_squared );
//			VECScale( p_mat_up, (Vec*)&temp, m_force.y * half_time_squared );
//			VECAdd( (Vec*)&at, (Vec*)&temp, (Vec*)&at );
//			VECScale( p_mat_at, (Vec*)&temp, m_force.z * half_time_squared );
//			VECAdd( (Vec*)&at, (Vec*)&temp, (Vec*)&at );
//
//			pos.set( p_matrix->getPosX() + p.x + ut.x + at.x, p_matrix->getPosY() + p.y + ut.y + at.y, p_matrix->getPosZ() + p.z + ut.z + at.z );
//			VECAdd((Vec*)&p, (Vec*)&ut, (Vec*)&pos );
//			VECAdd((Vec*)&pos, (Vec*)&mat_pos, (Vec*)&pos );
//			VECAdd((Vec*)&pos, (Vec*)&at, (Vec*)&pos );
//
//			// Deal with color change.
//			GXColor color;
//			color.r = (int)( m_gx_start_color_r + ( m_gx_delta_color_r * p_part->t ));
//			color.g = (int)( m_gx_start_color_g + ( m_gx_delta_color_g * p_part->t ));
//			color.b = (int)( m_gx_start_color_b + ( m_gx_delta_color_b * p_part->t ));
//			color.a = ((int)( m_gx_start_color_a + ( m_gx_delta_color_a * p_part->t ))) / 2;
//
//			if( mp_material )
//			{
//				NsRender::setBlendMode(	mp_material->getBlendMode(), p_texture, color.a );
//			}
//
//			// Deal with particle growth.
//			float		width	= ( m_size + ( m_size * m_growth * p_part->t )) * 0.5f;
//			float		height	= width * m_aspect_ratio;
//			NsVector	ss_right, ss_up, ss_pos;
//			ss_right.scale( screen_right, width );
//			ss_up.scale( screen_up, height );
//			VECScale((Vec*)&screen_right, (Vec*)&ss_right, width );
//			VECScale((Vec*)&screen_up, (Vec*)&ss_up, height );
//
//			// Set material color.
//			GX::SetChanMatColor( GX_COLOR0A0, color );
//
//			// Send coordinates.
//		    GXBegin( GX_QUADS, GX_VTXFMT3, 4 ); 
//			ss_pos.sub( pos, ss_right );
//			ss_pos.sub( ss_up );
//			VECSubtract((Vec*)&pos, (Vec*)&ss_right, (Vec*)&ss_pos );
//			VECSubtract((Vec*)&ss_pos, (Vec*)&ss_up, (Vec*)&ss_pos );
//			GXPosition3f32( ss_pos.x, ss_pos.y, ss_pos.z );
//			GXTexCoord2f32( 0.0f, 0.0f );
//
//			ss_pos.add( pos, ss_right );
//			ss_pos.sub( ss_up );
//			VECAdd((Vec*)&pos, (Vec*)&ss_right, (Vec*)&ss_pos );
//			VECSubtract((Vec*)&ss_pos, (Vec*)&ss_up, (Vec*)&ss_pos );
//			GXPosition3f32( ss_pos.x, ss_pos.y, ss_pos.z );
//			GXTexCoord2f32( 1.0f, 0.0f);
//
//			ss_pos.add( pos, ss_right );
//			ss_pos.add( ss_up );
//			VECAdd((Vec*)&pos, (Vec*)&ss_right, (Vec*)&ss_pos );
//			VECAdd((Vec*)&ss_pos, (Vec*)&ss_up, (Vec*)&ss_pos );
//			GXPosition3f32( ss_pos.x, ss_pos.y, ss_pos.z );
//			GXTexCoord2f32( 1.0f, 1.0f);
//
//			ss_pos.sub( pos, ss_right );
//			ss_pos.add( ss_up );
//			VECSubtract((Vec*)&pos, (Vec*)&ss_right, (Vec*)&ss_pos );
//			VECAdd((Vec*)&ss_pos, (Vec*)&ss_up, (Vec*)&ss_pos );
//			GXPosition3f32( ss_pos.x, ss_pos.y, ss_pos.z );
//			GXTexCoord2f32( 0.0f, 1.0f);
//			GXEnd();
//		}
//		NsRender::end();
//	}
//	m_time_since_last_draw = 0.0f;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::setMaterial( NsMaterial* p_material )
//{
//	if( mp_material )
//	{
//		delete mp_material;
//	}
//
//	mp_material = new NsMaterial;
//	memcpy( mp_material, p_material, sizeof( NsMaterial ));	
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::SetEmitterSize( float w, float h )
//{
//	m_emitter_width		= w * 0.5f;
//	m_emitter_height	= h * 0.5f;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::SetEmitterAngle( float angle )
//{
//	// Convert to radians.
//	angle *= Mth::PI / 180.0f;
//
//	// Convert to range [0, 1].
//    m_emitter_angle = angle / ( Mth::PI * 0.5f );
//
//	m_flags |= ( PARTICLE_FLAG_DIRTY | PARTICLE_FLAG_BBOX_DIRTY );
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::setParticleColors( uint32* start, uint32* end )
//{
//	m_start_color	= *start;
//	m_end_color		= *end;
//
//	m_gx_start_color_r	= (float)((int)( m_start_color & 0xFF ));
//	m_gx_start_color_g	= (float)((int)(( m_start_color >> 8 ) & 0xFF ));
//	m_gx_start_color_b	= (float)((int)(( m_start_color >> 16 ) & 0xFF ));
//	m_gx_start_color_a	= (float)((int)(( m_start_color >> 24 ) & 0xFF ));
//
//	m_gx_delta_color_r	= (float)((int)( m_end_color & 0xFF )) - m_gx_start_color_r;
//	m_gx_delta_color_g	= (float)((int)(( m_end_color >> 8 ) & 0xFF )) - m_gx_start_color_g;
//	m_gx_delta_color_b	= (float)((int)(( m_end_color >> 16 ) & 0xFF )) - m_gx_start_color_b;
//	m_gx_delta_color_a	= (float)((int)(( m_end_color >> 24 ) & 0xFF )) - m_gx_start_color_a;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//uint32 NsParticleAtomic::getParticleColors( uint32 index )
//{
//	if( index == 0 )
//		return m_start_color;
//	else
//		return m_end_color;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::setParticleSpeed( float min, float max, float damping )
//{
//	m_min_speed			= ( min + max ) * 0.5f;
//	m_variation_speed	= max - m_min_speed;
//	m_damping			= -damping;
//
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::setParticleFlightTime( float t )
//{
//	m_flight_time		= t;
//	m_inv_flight_time	= 1.0f / t;
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::setParticleForce( NsVector& f )
//{
//	// Scaled here for the 1/2at^2 calc.
//	m_force.scale( f, 0.5f );
//}
//
//
//
///******************************************************************/
///*                                                                */
///*                                                                */
///******************************************************************/
//void NsParticleAtomic::setParticleSize( float size, float growth, float aspect_ratio )
//{
//	m_size			= size;
//	m_growth		= growth;
//	m_aspect_ratio	= aspect_ratio;
//}
//
//} // namespace NsParticle
