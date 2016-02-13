#ifndef	__GFX_NXMISCFX_H__
#define	__GFX_NXMISCFX_H__

#include <gfx/nx.h>

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
enum eScreenFlashFlags
{
	SCREEN_FLASH_FLAG_BEHIND_PANEL	= 0x01,
	SCREEN_FLASH_FLAG_ADDITIVE		= 0x02,
	SCREEN_FLASH_FLAG_SUBTRACTIVE	= 0x04,
	SCREEN_FLASH_FLAG_IGNORE_PAUSE	= 0x08
};
	

	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sScreenFlashDetails
{
	int			m_viewport;
	Image::RGBA	m_from;
	Image::RGBA	m_to;
	Image::RGBA	m_current;
	uint32		m_flags;
	float		m_duration;
	float		m_lifetime;
	float		m_z;
	CTexture*	mp_texture;
};


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
class CFog
{
public:
	static void			sEnableFog(bool enable);
	static void			sSetFogExponent(float exponent);
	static void			sSetFogNearDistance(float distance);
	static void			sSetFogRGBA(Image::RGBA rgba);
	static void			sSetFogColor( void );
	static void			sFogUpdate( void );

	static bool			sIsFogEnabled();
	static float		sGetFogNearDistance();
	static float		sGetFogExponent();
	static Image::RGBA	sGetFogRGBA();

private:
	static void			s_plat_enable_fog(bool enable);
	static void			s_plat_set_fog_near_distance(float distance);
	static void			s_plat_set_fog_exponent(float exponent);
	static void			s_plat_set_fog_rgba(Image::RGBA rgba);
	static void			s_plat_set_fog_color( void );
	static void			s_plat_fog_update( void );

	// Keep the numbers here, just in case they need to be mangled below the p-line
	static bool			s_enabled;
	static float		s_near_distance;
	static float		s_exponent;
	static Image::RGBA	s_rgba;
};


#define SPLAT_POLYS_PER_MESH	256
	
/******************************************************************/
/*                                                                */
/* Platform independent structure for holding details on a set    */
/* of texture splats. The platform-specific code will most likely */
/* derive a structure from this to maintain additional details.   */
/*                                                                */
/******************************************************************/
struct sSplatInstanceDetails
{
	int					m_lifetimes[SPLAT_POLYS_PER_MESH];
	int					m_highest_active_splat;				// Can be used to dynamically optimise rendering lists.
	int					GetOldestSplat( void );
};



/******************************************************************/
/*                                                                */
/* Platform independent structure for holding details on a single */
/* trail texture splats.										  */
/*                                                                */
/******************************************************************/
struct sSplatTrailInstanceDetails
{
	Mth::Vector			m_last_pos;
	uint32				m_trail_id;				// Keyed from the bone checksum.
	uint32				m_last_pos_added_time;	// In milliseconds.
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sShatterInstanceDetails
{
						sShatterInstanceDetails( int num_tris );
	virtual				~sShatterInstanceDetails( void );
	void				UpdateParameters( int index, float timestep );
	
	Mth::Vector*		mp_positions;
	Mth::Vector*		mp_velocities;
	Mth::Vector*		mp_normals;    // For 'fake' lighting.
	Mth::Matrix*		mp_matrices;
	float				m_lifetime;	
	float				m_gravity;	
	float				m_bounce_level;
	float				m_bounce_amplitude;
	int					m_num_triangles;

protected:
	static int			s_query_memory_needed(int num_tris);
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sTriSubdivideStack
{
	static const int	TRI_SUBDIVIDE_STACK_SIZE	= 16 * 1024;

	void				Reset( void );
	void				Clear( void );
	bool				IsEmpty( void )				{ return m_offset == 0; }
	void				SetBlockSize( int size )	{ m_block_size = size; }
	int					GetBlockSize( void )		{ return m_block_size; }
	void				Pop( void *p_data );
	void				Push( void *p_data );
	const void *		Peek( uint index );

	private:	
	int					m_offset;
	int					m_block_size;
	char				m_data[TRI_SUBDIVIDE_STACK_SIZE];
};



extern Lst::HashTable< sSplatInstanceDetails >			*p_splat_details_table;
extern Lst::HashTable< sSplatTrailInstanceDetails >		*p_splat_trail_details_table;
extern Lst::HashTable< sShatterInstanceDetails >		*p_shatter_details_table;
extern Mth::Vector										shatterVelocity;
extern float											shatterAreaTest;
extern float											shatterVelocityVariance;
extern float											shatterSpreadFactor;
extern float											shatterLifetime;
extern float											shatterBounce;

extern sTriSubdivideStack								triSubdivideStack;

void MiscFXInitialize( void );
void MiscFXCleanup( void );

void AddScreenFlash( int viewport, Image::RGBA from, Image::RGBA to, float duration, float z, uint32 flags, const char *p_texture_name );
void ScreenFlashUpdate( void );
void ScreenFlashRender( int viewport, uint32 flags );

void TextureSplatRender( void );
void TextureSplatUpdate( void );
bool TextureSplat( Mth::Vector& splat_start, Mth::Vector& splat_end, float size, float lifetime, const char *p_texture_name, uint32 trail = 0 );
void KillAllTextureSplats( void );

void ShatterSetParams( Mth::Vector& velocity, float area_test, float velocity_variance, float spread_factor, float lifetime, float bounce, float bounce_amplitude );
void Shatter( CGeom *p_geom );
void ShatterUpdate( void );
void ShatterRender( void );

void plat_screen_flash_render( sScreenFlashDetails *p_details );

void plat_texture_splat_initialize( void );
void plat_texture_splat_cleanup( void );
void plat_texture_splat_render( void );
void plat_texture_splat_reset_poly( sSplatInstanceDetails *p_details, int index );
bool plat_texture_splat( Nx::CSector **pp_sectors, Nx::CCollStatic **pp_collision, Mth::Vector& start, Mth::Vector& end, float size, float lifetime, Nx::CTexture *p_texture, Nx::sSplatTrailInstanceDetails *p_trail_details = NULL );

void plat_shatter_initialize( void );
void plat_shatter_cleanup( void );
void plat_shatter( CGeom *p_geom );
void plat_shatter_update( sShatterInstanceDetails *p_details, float framelength );
void plat_shatter_render( sShatterInstanceDetails *p_details );

} // Nx

#endif // __GFX_NXMISCFX_H__
