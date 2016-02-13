#ifndef	__GFX_P_NX_SECTOR_H__
#define	__GFX_P_NX_SECTOR_H__

#include 	<core\math.h>
#include 	<core\math\geometry.h>
#include 	"gfx\NxSector.h"
#include 	"gfx\Image\ImageBasic.h"

#include 	"gfx\xbox\p_nxscene.h"
#include 	"gfx\xbox\nx\mesh.h"

namespace Nx
{

/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//
// Here's a machine specific implementation of the CSector
class CXboxSector : public CSector
{
	public:
									CXboxSector();

	bool							LoadFromFile( void* p_file );
	bool							LoadFromMemory( void **p_mem );

	NxXbox::sScene					*GenerateScene( void );
	
	private:						// It's all private, as it is machine specific
	virtual void					plat_set_color( Image::RGBA rgba );
	virtual void					plat_clear_color( void );
	virtual void					plat_set_visibility( uint32 mask );
	virtual void					plat_set_active( bool on );
	virtual void					plat_set_world_position( const Mth::Vector& pos );
	virtual const Mth::CBBox		&plat_get_bounding_box( void ) const;
	virtual const Mth::Vector		&plat_get_world_position( void ) const;
	virtual void					plat_set_shatter( bool on );
	virtual CSector					*plat_clone( bool instance, CScene *p_scene = NULL );

	int								m_flags;

	Mth::Vector						m_pos_offset;

	Image::RGBA						m_rgba;
};

} // Namespace Nx  			

#endif