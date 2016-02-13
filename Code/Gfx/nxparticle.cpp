///////////////////////////////////////////////////////////////////////////////
// NxParticle.cpp

#include <core/defines.h>
#include <core/singleton.h>
#include <core/math.h>
#include <gfx/NxParticle.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/script.h>
#include <sk/modules/frontend/frontend.h>
#include <gel/components/suspendcomponent.h>
#include <gfx/nx.h>
#include <gfx/debuggfx.h>
#include <sys/replay/replay.h>

#define next_random() ((((float)rand() / RAND_MAX ) * 2.0f ) - 1.0f)

//NsVector pright;
//NsVector pup;
//NsVector pat;

namespace	Nx
{
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CParticle::plat_render( void )
{
	printf ("STUB: plat_render\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CParticle::plat_get_position( int entry, int list, float * x, float * y, float * z )
{
	printf ("STUB: plat_get_position\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CParticle::plat_set_position( int entry, int list, float x, float y, float z )
{
	printf ("STUB: plat_set_position\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CParticle::plat_add_position( int entry, int list, float x, float y, float z )
{
	printf ("STUB: plat_add_position\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CParticle::plat_set_active( bool active )
{
	// PS2 & Gamecube do nothing here.
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
void CParticle::plat_build_path( void )
{
	printf ("STUB: plat_build_path\n");
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
	
int CParticle::plat_get_num_particle_colors( void ) { printf ("STUB: plat_get_num_particle_colors\n"); return 0; };
int CParticle::plat_get_num_vertex_lists( void ) { printf ("STUB: plat_get_num_vertex_lists\n"); return 0; };
void CParticle::plat_set_sr( int entry, uint8 value ) { printf ("STUB: plat_set_sr\n"); }
void CParticle::plat_set_sg( int entry, uint8 value ) { printf ("STUB: plat_set_sg\n"); }
void CParticle::plat_set_sb( int entry, uint8 value ) { printf ("STUB: plat_set_sb\n"); }
void CParticle::plat_set_sa( int entry, uint8 value ) { printf ("STUB: plat_set_sa\n"); }
void CParticle::plat_set_mr( int entry, uint8 value ) { printf ("STUB: plat_set_mr\n"); }
void CParticle::plat_set_mg( int entry, uint8 value ) { printf ("STUB: plat_set_mg\n"); }
void CParticle::plat_set_mb( int entry, uint8 value ) { printf ("STUB: plat_set_mb\n"); }
void CParticle::plat_set_ma( int entry, uint8 value ) { printf ("STUB: plat_set_ma\n"); }
void CParticle::plat_set_er( int entry, uint8 value ) { printf ("STUB: plat_set_er\n"); }
void CParticle::plat_set_eg( int entry, uint8 value ) { printf ("STUB: plat_set_eg\n"); }
void CParticle::plat_set_eb( int entry, uint8 value ) { printf ("STUB: plat_set_eb\n"); }
void CParticle::plat_set_ea( int entry, uint8 value ) { printf ("STUB: plat_set_ea\n"); }

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CParticleEntry::CParticleEntry()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CParticleEntry::~CParticleEntry()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CParticle::set_defaults( void )
{
#define px 0x800		//0x4a1;
#define py 0x90			//-0x117;		//0xfffffee9;
#define pz -0x3a0		//0xb96;

	mp_emit_script = NULL;
	mp_update_script = NULL;
	mp_params = NULL;
	m_life_set = false;
	m_end_set = false;


	// Temp: Set up some debug values.
	m_pos[X] = px;
	m_pos[Y] = py;
	m_pos[Z] = pz;

	m_sw = 2.0f;
	m_sh = 2.0f;
	m_ew = 2.0f;
	m_eh = 2.0f;
	
	m_life_min = 2.0f;
	m_life_max = 2.0f;

	m_emit_w = 1.0f;	//16.0f;
	m_emit_h = 1.0f;    //16.0f;
	m_emit_angle_spread = 0.5f;		//0.5f;

	m_fx = 0.0f;
	m_fy = -0.02f;
	m_fz = 0.0f;

	m_tx = 0.0f;		// Default target: straight up.
	m_ty = 1.0f;
	m_tz = 0.0f;

	m_ax = 0.0f;		// Default angles: No rotation.
	m_ay = 0.0f;
	m_az = 0.0f;

	m_speed_min = 1.0f;
	m_speed_max = 2.0f;

	m_random_angle = false;
	m_circular_emit = true;

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CParticle::CParticle()
{
	set_defaults();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CParticle::CParticle( uint32 checksum )
{
	set_defaults();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CParticle::CParticle( uint32 checksum, int maxParticles )
{
	set_defaults();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CParticle::~CParticle()
{
	if ( mp_emit_script )
	{
		delete mp_emit_script;
	}
	if ( mp_update_script )
	{
		delete mp_update_script;
	}
	if ( mp_params )
	{
		delete mp_params;
	}
	CEngine::sGetParticleTable()->FlushItem( m_checksum );		// remove reference from hash table
//	printf ("\nJust Deleted 0x%x, contenets are now:\n",m_checksum);
//	CEngine::sGetParticleTable()->PrintContents();
}

void CParticle::process( float delta_time )
{
	int				lp;
	CParticleEntry*	p_particle;

	// If flagged for deletion, check if the particle system is empty and delete if it is.
	if ( m_delete_when_empty )
	{
		if ( !m_num_particles )
		{
			// removing reference is sone in the destructor
			//CEngine::sGetParticleTable()->FlushItem( m_checksum );		// remove reference from hash table
			delete this;
			return;
		}
	}

	// Don't process if paused.
	if (Mdl::FrontEnd::Instance()->GamePaused() && !Replay::RunningReplay())
	{
		return;
	}	
	if (Replay::Paused())
	{
		return;
	}	
	
	if ( GetSuspendComponent()->SkipLogic( ) )
	{
		return;
	}
	
	

	// Script updates.
	if ( mp_update_script )
	{
		mp_update_script->Update();
	}

// We can't check for off screen or not until we've run the update script
// as that might change the position (like for sparks)

	// Don't process if off screen
//	if (!Nx::CEngine::sIsVisible(m_pos,m_bounding_radius))
//	{
//		//printf ("Off - process\n");
//		Gfx::AddDebugLine(m_pos,Mth::Vector(0,0,0),0x0000ff,0x0000ff,100);	 	// where is it?
//		return;
//	}


	if ( mp_emit_script )
	{
		mp_emit_script->Update();
//		if ( mp_emit_script->Update() == Script::ESCRIPTRETURNVAL_FINISHED )
//		{
//			m_delete_when_empty = true;
//		}
	}



	// If life was not set, assume we're a screen-facing system.
	if ( !m_life_set ) return;

	// Physics update.
	for ( lp = 0, p_particle = mp_particle_array; lp < m_num_particles; lp++, p_particle++ )
	{
		// Update time/life.
		p_particle->m_time += delta_time;

		// Apply force to velocity.
		p_particle->m_vx += p_particle->m_fx;
		p_particle->m_vy += p_particle->m_fy;
		p_particle->m_vz += p_particle->m_fz;

		// Copy history.
		float x, y, z;
		for ( int lp2 = ( plat_get_num_vertex_lists() - 1 ); lp2 > 0; lp2-- )
		{
			plat_get_position( lp, lp2 - 1, &x, &y, &z );
			plat_set_position( lp, lp2, x, y, z );
		}

		// Apply velocity to position.
		plat_add_position( lp, 0, p_particle->m_vx, p_particle->m_vy, p_particle->m_vz );
	}

	// Delete old particles.
	for ( lp = 0, p_particle = mp_particle_array; lp < m_num_particles; lp++, p_particle++ )
	{
		if ( p_particle->m_time > p_particle->m_life )
		{
			// Delete old particles by copying last entry to entry that is being deleted.
			m_num_particles--;
			memcpy( p_particle, &mp_particle_array[m_num_particles], sizeof( CParticleEntry ) );

			float x, y, z;
			for ( int lp2 = 0; lp2 < plat_get_num_vertex_lists(); lp2++ )
			{
				plat_get_position( m_num_particles, lp2, &x, &y, &z );
				plat_set_position( lp, lp2, x, y, z );
				plat_set_position( m_num_particles, lp2, 1024.0f, 1024.0f, 1024.0f );
			}

			// Since we copied the last one to the current slot, decrement lp counter & particle
			// pointer so that the copied entry gets checked for deletion as well.
			p_particle--;
			lp--;
		}
	}
}

void CParticle::render( void )
{

	if ( GetSuspendComponent()->SkipRender( ) )
	{
		return;
	}

	plat_render();
}

void CParticle::emit( int count )
{
	int first, last;

	// Do nothing here if flagged for deletion.
	if ( m_delete_when_empty )
	{
		return;
	}

	// Calculate the range of array entries to emit to.
	first = m_num_particles;
	if ( ( m_num_particles + count ) <= m_max_particles )
	{
		m_num_particles += count;
	}
	else
	{
		m_num_particles = m_max_particles;
	}
	last = m_num_particles;

	// Build matrix to rotate the particle to the target.
	Mth::Matrix mt;

	mt.GetPos().Set();
	mt.GetAt().Set( m_tx, m_ty, m_tz );
	mt.GetAt().Normalize();

	Mth::Vector temp;
	temp.Set( 0.0f, 1.0f, 0.0f );
	if ( ( fabs( mt.GetAt()[Y] ) > fabs( mt.GetAt()[X] ) ) && ( fabs( mt.GetAt()[Y] ) > fabs( mt.GetAt()[Z] ) ) )
	{
		// Y Major - use Z as up.
		temp.Set( 0.0f, 0.0f, 1.0f );
	}

	mt.GetRight() = Mth::CrossProduct( temp, mt.GetAt() ); 
	mt.GetRight().Normalize();
	mt.GetUp() = Mth::CrossProduct( mt.GetRight(), mt.GetAt() ); 
	mt.GetUp().Normalize();

	// Add on rotation.
	mt.RotateX( Mth::DegToRad(m_ax) );
	mt.RotateY( Mth::DegToRad(m_ay) );
	mt.RotateZ( Mth::DegToRad(m_az) );

//	pright.set( mt.GetRight()[X], mt.GetRight()[Y], mt.GetRight()[Z] );
//	pup.set( mt.GetUp()[X], mt.GetUp()[Y], mt.GetUp()[Z] );
//	pat.set( mt.GetAt()[X], mt.GetAt()[Y], mt.GetAt()[Z] );

	// Emit particles.
	for ( int lp = first; lp < last; lp++ )
	{
		if ( m_life_set )
		{
			// Random values for this emission.
			float rx = next_random();
			float ry = next_random();

			// Calculate emission initial velocity.		
			Mth::Vector velocity;
			velocity[X] = rx * ( Mth::PI * ( m_emit_angle_spread / 4.0f ) );
			velocity[Y] = ry * ( Mth::PI * ( m_emit_angle_spread / 4.0f ) );  
			velocity[Z] = 1.0f - (( velocity[X] * velocity[X] ) + ( velocity[Y] * velocity[Y] ));
			velocity[W] = 1.0f;

			float speed_mul = ( ( m_speed_max - m_speed_min ) * ((float)rand() / RAND_MAX ) ) + m_speed_min;
			velocity[X] *= speed_mul;
			velocity[Y] *= speed_mul;
			velocity[Z] *= speed_mul;

			// Get new rx, ry to make the particles cross over one another.
			if ( m_random_angle )
			{
				rx = next_random();
				ry = next_random();
			}

			// Normalize rx, ry to make a circular emission.
			if ( m_circular_emit )
			{
				float l = 1.0f / sqrtf( ( rx * rx ) + ( ry * ry ) );
				rx = rx * ( l * (float)fabs( rx ) );		// Note: length is scaled by random factor.
				ry = ry * ( l * (float)fabs( ry ) );
			}

			// Set position around emitter width/height.
			Mth::Vector pos;
			pos[X] = m_emit_w * rx;
			pos[Y] = m_emit_h * ry;
			pos[Z] = 0.0f;
			pos[W] = 1.0f;

			// Set force.
			mp_particle_array[lp].m_fx = m_fx;
			mp_particle_array[lp].m_fy = m_fy;
			mp_particle_array[lp].m_fz = m_fz;

			// Set particle w/h
			mp_particle_array[lp].m_sw = m_sw;
			mp_particle_array[lp].m_sh = m_sh;
			mp_particle_array[lp].m_ew = m_ew;
			mp_particle_array[lp].m_eh = m_eh;

			// Set time & life.
			mp_particle_array[lp].m_time = 0.0f;
			mp_particle_array[lp].m_life = ( ( m_life_max - m_life_min ) * ((float)rand() / RAND_MAX ) ) + m_life_min;

			// Orient the particles to the target.
			Mth::Vector v;
			v = mt.Transform( velocity );
			mp_particle_array[lp].m_vx = v[X];
			mp_particle_array[lp].m_vy = v[Y];
			mp_particle_array[lp].m_vz = v[Z];
			
			v = mt.Transform( pos );

			for ( int lp2 = 0; lp2 < plat_get_num_vertex_lists(); lp2++ )
			{
				plat_set_position( lp, lp2, v[X], v[Y], v[Z] );
			}
		}
		else
		{
			// Set particle w/h
			mp_particle_array[lp].m_sw = m_sw;
			mp_particle_array[lp].m_sh = m_sh;
			mp_particle_array[lp].m_ew = m_ew;
			mp_particle_array[lp].m_eh = m_eh;

			// Set time & life.
			mp_particle_array[lp].m_time = 0.0f;
			mp_particle_array[lp].m_life = ( ( m_life_max - m_life_min ) * ((float)rand() / RAND_MAX ) ) + m_life_min;

			// Screen-facing poly. Just set to 0,0,0.
			for ( int lp2 = 0; lp2 < plat_get_num_vertex_lists(); lp2++ )
			{
				plat_set_position( lp, lp2, 0, 0, 0 );
			}
		}

	}
}

void CParticle::set_emit_script( uint32 checksum, Script::CStruct* pParams )
{
	if ( checksum )
	{
		if ( !mp_emit_script )
		{
			mp_emit_script = new Script::CScript();
			#ifdef __NOPT_ASSERT__
			mp_emit_script->SetCommentString("Created in CParticle::set_emit_script(...)");
			#endif
		}
		if ( !mp_params )
		{
			mp_params = new Script::CStruct;
		}
		mp_params->AppendStructure( pParams );
		mp_emit_script->SetScript( checksum, mp_params, this );
//		mp_emit_script->Update();
	}
}

void CParticle::set_update_script( uint32 checksum, Script::CStruct* pParams )
{
	if ( checksum )
	{
		if ( !mp_update_script )
		{
			mp_update_script = new Script::CScript();
			#ifdef __NOPT_ASSERT__
			mp_update_script->SetCommentString("Created in CParticle::set_update_script(...)");
			#endif
		}
		if ( !mp_params )
		{
			mp_params = new Script::CStruct;
		}
		mp_params->AppendStructure( pParams );
		mp_update_script->SetScript( checksum, mp_params, this );
	}
}


// Refresh a particle system after things like position have changed 

void CParticle::Refresh()
{
	plat_build_path();
}



void CParticle::SetActive( bool active )
{
	m_active = active;
	plat_set_active( active );
}

/******************************************************************/
/*   CParticle::CallMemberFunction                               */
/*   Call a member function, based on a checksum                  */
/*   This is usually the checksum of the name of the function     */
/*   but can actually be any number, as it just uses a switch     */
/*   note this is a virtual function, so the same checksum        */
/*   can do differnet things for different objects                */
/*                                                                */
/*                                                                */
/******************************************************************/

bool CParticle::CallMemberFunction( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript )
{
    
	Dbg_MsgAssert(pScript,("NULL pScript"));

	switch(Checksum)
	{
		// @script | SetPos | Sets the position of the particle system.
		// @parmopt vector | pos | world position.
		// @parmopt float | x	| x world position.
		// @parmopt float | y	| y world position.
		// @parmopt float | z	| z world position.
		case 0x99cd17b:		// SetPos
			{
				Mth::Vector pos;
				float x = 0.0f;
				float y = 0.0f;
				float z = 0.0f;
				if ( pParams->GetVector(CRCD(0x7f261953,"pos"),&pos) )
				{
					x = pos[X];
					y = pos[Y];
					z = pos[Z];
				}
				else
				{
					pParams->GetFloat(CRCD(0x7323e97c,"x"),&x);
					pParams->GetFloat(CRCD(0x424d9ea,"y"),&y);
					pParams->GetFloat(CRCD(0x9d2d8850,"z"),&z);
				}

				set_pos( x, y, z );
			}
			break;
		// @script | SetSpeedRange | Sets the min & max speed of the particles upon emission.
		// @parm float | min | minimum speed. If only min is provided, all speeds will be set to min.
		// @parmopt float | max | maximum speed.
		case 0x63274395:		// SetSpeedRange
			{
				float min = 0.0f;
				pParams->GetFloat(CRCD(0x5e84e22f,"min"),&min);
				float max = min;
				pParams->GetFloat(CRCD(0x6289dd76,"max"),&max);

				set_speed_range( min, max );
			}
			break;
		// @script | SetEmitRange | Sets the emission range (width and height).
		// @parm float | width | Width of emission area. If only width is provided, height will be set to width.
		// @parmopt float | height | Height of emission area.
		case 0x8ffdef38:		// SetEmitRange
			{
				float width = 0.0f;
				pParams->GetFloat(CRCD(0x73e5bad0,"width"),&width);
				float height = width;
				pParams->GetFloat(CRCD(0xab21af0,"height"),&height);
			
				set_emit_range( width, height );
			}
			break;
		// @script | SetAngleSpread | Sets the angle spread for particle emissions.
		// @parm float | spread | Angle spread. 0=No spread. 1=Full spread (180 degrees).
		case 0x7911855b:		// SetAngleSpread
			{
				float spread = 0.0f;
				pParams->GetFloat(CRCD(0x834a5f87,"spread"),&spread);
			
				set_emit_angle_spread( spread );
			}
		break;
	

		// @script | SetRandomAngle | Sets whether the emission angle is random or not.
		// @parm int | random | 1 means choose a random angle regardless of the emission position. 0 means use the emission position to define the emission angle.
		case 0xdb8621f7:		// SetRandomAngle
			{
				int random = 0;
				pParams->GetInteger("random",(int*)&random);
			
				set_random_angle( random > 0 );
			}
			break;
		
		// @script | SetCircularEmit | Sets whether the emission area is circular or square.
		// @parm int | circular | 1 means use a circular emission area. 0 means use a square emission area.
		case 0xe985a9a6:		// SetCircularEmit
			{
				int circular = 0;
				pParams->GetInteger("circular",(int*)&circular);
			
				set_circular_emit( circular > 0 );
			}
			break;
		
		// @script | SetForce | Sets the force acting on the particles.
		// @parmopt vector | force	| Force.
		// @parmopt float | x	| x force.
		// @parmopt float | y	| y force.
		// @parmopt float | z	| z force.
		case 0xa075ee45:		// SetForce
			{
				Mth::Vector force;
				float x = 0.0f;
				float y = 0.0f;
				float z = 0.0f;
				if ( pParams->GetVector("force",&force) )
				{
					x = force[X];
					y = force[Y];
					z = force[Z];
				}
				else
				{
					pParams->GetFloat("x",&x);
					pParams->GetFloat("y",&y);
					pParams->GetFloat("z",&z);
				}

				set_force( x, y, z );
			}
			break;
			
		// @script | SetParticleSize | Sets the size of the particles.
		// @parm float | sw | Start width of the particle. If only width is provided, height is set to width.
		// @parmopt float | sh | Start height of the particle.
		// @parm float | ew | End width of the particle. If only width is provided, height is set to width.
		// @parmopt float | eh | End height of the particle.
		case 0x2a479569:		// SetParticleSize
			{
				float sw = 0.0f;
				pParams->GetFloat("sw",&sw);
				float sh = sw;
				pParams->GetFloat("sh",&sh);
			
				set_particle_start_size( sw, sh );

				float ew = sw;
				float eh;
				if ( pParams->GetFloat("ew",&ew) )
				{
					eh = ew;
				}
				else
				{
					eh = sh;
				}
				pParams->GetFloat("eh",&eh);
			
				set_particle_end_size( ew, eh );
			}
			break;
		
		// @script | SetLife | Sets the life of the particles in seconds.
		// @parm float | min | Minimum life of the particle. 1.0 = 1 second.
		// @parmopt float | max | Maximum life of the particle. 1.0 = 1 second. If not specified, will be set to same as min.
		case 0xd3f1d333:		// SetLife
			{
				float min = 0.0f;
				pParams->GetFloat("min",&min);
				float max = min;
				pParams->GetFloat("max",&max);
			
				set_particle_life( min, max );
			}
			break;

		// @script | SetEmitTarget | Sets the target position that the particle system will emit particles towards.
		// @parmopt vector | target	| Target.
		// @parmopt float | x	| x target.
		// @parmopt float | y	| y target.
		// @parmopt float | z	| z target.
		case 0xb373fb6e:		// SetEmitTarget
			{
				Mth::Vector target;
				float x = 0.0f;
				float y = 0.0f;
				float z = 0.0f;
				if ( pParams->GetVector("target",&target) )
				{
					x = target[X];
					y = target[Y];
					z = target[Z];
				}
				else
				{
					pParams->GetFloat("x",&x);
					pParams->GetFloat("y",&y);
					pParams->GetFloat("z",&z);
				}
				
				// If it's just 0,1,0, then rotate it by the angles of the node
				if (x == 0.0f && y == 1.0f && z == 0.0f)
				{
					// not done here though....
					
				}
				else
				{
					set_emit_target( x, y, z );
				}
				

			}
			break;
		// @script | SetEmitAngle | Sets the emission angle. This is done after the target, so you can set a target as well as angles to rotate in addition to the target angle. Target defaults to 0,1,0 (straight up), so all angles rotate from there.
		// @parmopt vector | angle	| Angle (in degrees).
		// @parmopt float | x	| x angle.
		// @parmopt float | y	| y angle.
		// @parmopt float | z	| z angle.
		case 0x1cfbf078:		// SetEmitAngle
			{
				Mth::Vector angle;
				float x = 0.0f;
				float y = 0.0f;
				float z = 0.0f;
				if ( pParams->GetVector("angle",&angle) )
				{
					x = angle[X];
					y = angle[Y];
					z = angle[Z];
				}
				else
				{
					pParams->GetFloat("x",&x);
					pParams->GetFloat("y",&y);
					pParams->GetFloat("z",&z);
				}

				set_emit_angle( x, y, z );
			}
			break;
		// CreateParticleSystem colorentries=n	// default 2.
		// SetColorEntry slot=n r=n g=n b=n


		// @script | SetColor | Sets the color for the next particle emission.
		// @parmopt int | corner | | The corner to set. Omitting this paramter means set all corners.
		// @parmopt int | sr | | Start red color element to set (0-255). 128 is normal.
		// @parmopt int | sg | | Start green color element to set (0-255). 128 is normal.
		// @parmopt int | sb | | Start blue color element to set (0-255). 128 is normal.
		// @parmopt int | sa | | Start alpha color element to set (0-255). 255 is opaque, 0 is invisible.
		// @parmopt int | mr | | Mid red color element to set (0-255). 128 is normal.
		// @parmopt int | mg | | Mid green color element to set (0-255). 128 is normal.
		// @parmopt int | mb | | Mid blue color element to set (0-255). 128 is normal.
		// @parmopt int | ma | | Mid alpha color element to set (0-255). 255 is opaque, 0 is invisible.
		// @parmopt int | er | | End red color element to set (0-255). 128 is normal.
		// @parmopt int | eg | | End green color element to set (0-255). 128 is normal.
		// @parmopt int | eb | | End blue color element to set (0-255). 128 is normal.
		// @parmopt int | ea | | End alpha color element to set (0-255). 255 is opaque, 0 is invisible.
		// @parmopt int | midtime | | Sets the % through the particle's life that the mid color is reached.
		// 0.5 means halfway through. Use values from 0-1. By default, the mid color is not used. By setting
		// the mid time, the middle color will be used.
		case 0x514b2584:		// SetColor
			{
				int corner;
				int first = 0;
				int last = 0;
				if ( pParams->GetInteger("corner",&corner) )
				{
					first = last = corner;
				}
				else
				{
					first = 0;
					last = plat_get_num_particle_colors() - 1;
				}
				if ( ( first < plat_get_num_particle_colors() ) && ( last < plat_get_num_particle_colors() ) )
				{
					for ( int entry = first; entry < ( last + 1 ); entry++ )
					{
						if ( entry < plat_get_num_particle_colors() )
						{
							int elem;
							if ( pParams->GetInteger("er",&elem) )
							{
								plat_set_er( entry, elem );
								m_end_set = true;
							}
							if ( pParams->GetInteger("eg",&elem) )
							{
								plat_set_eg( entry, elem );
								m_end_set = true;
							}
							if ( pParams->GetInteger("eb",&elem) )
							{
								plat_set_eb( entry, elem );
								m_end_set = true;
							}
							if ( pParams->GetInteger("ea",&elem) )
							{
								plat_set_ea( entry, elem );
								m_end_set = true;
							}
							if ( pParams->GetInteger("mr",&elem) )
							{
								plat_set_mr( entry, elem );
							}
							if ( pParams->GetInteger("mg",&elem) )
							{
								plat_set_mg( entry, elem );
							}
							if ( pParams->GetInteger("mb",&elem) )
							{
								plat_set_mb( entry, elem );
							}
							if ( pParams->GetInteger("ma",&elem) )
							{
								plat_set_ma( entry, elem );
							}
							if ( pParams->GetInteger("sr",&elem) )
							{
								plat_set_sr( entry, elem );
								if ( !m_end_set ) plat_set_er( entry, elem ); 
							}
							if ( pParams->GetInteger("sg",&elem) )
							{
								plat_set_sg( entry, elem );
								if ( !m_end_set ) plat_set_eg( entry, elem ); 
							}
							if ( pParams->GetInteger("sb",&elem) )
							{
								plat_set_sb( entry, elem );
								if ( !m_end_set ) plat_set_eb( entry, elem ); 
							}
							if ( pParams->GetInteger("sa",&elem) )
							{
								plat_set_sa( entry, elem );
								if ( !m_end_set ) plat_set_ea( entry, elem ); 
							}
						}
					}
				}
				pParams->GetFloat("midtime",&m_mid_time);
			}
			break;
		// @script | Emit | Emits the specified number of particles.
		// @parm int | num | Number of particles to emit.
		case 0x711b1a6a:		// Emit
			{
				int num = 1;		// Defaults to 1 particle emitted.
				pParams->GetInteger("num",&num);

				emit( num );
			}
			break;
		// @script | EmitRate | Emits at the specified rate - 1=1 particle per frame.
		// @parm float | rate | The rate to emit at.
		case 0x35b65bd4:		// EmitRate
			{
				float rate = 1.0f;		// Defaults to 1 particle emitted per frame.
				pParams->GetFloat("rate",&rate);

				set_emit_rate( rate );
			}
			break;
		// @script | BuildPath | Builds a path based on the current physics data.
		case 0xa8699e0e:		// BuildPath
			{
				plat_build_path();
			}
			break;

		// @script | EmptySystem | Empties the particle system immediately (sets number of particles to 0.
		case 0x482482c6:		// EmptySystem
			{
				m_num_particles = 0;
			}
			break;

		default:
			return Obj::CMovingObject::CallMemberFunction( Checksum, pParams, pScript );
	}

	return true;
}

}

