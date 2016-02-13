///////////////////////////////////////////////////////////////////////////////
// p_NxFont.h

#ifndef	__GFX_P_NX_IMPOSTER_H__
#define	__GFX_P_NX_IMPOSTER_H__

#include 	"gfx/NxImposter.h"

namespace Nx
{

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sImposterPolyVert
{
	float		x, y, z;
	float		u, v;
};
	
	
/////////////////////////////////////////////////////////////////////////////////////
// Private classes
//

struct sRemovedTextureDetails
{
	IDirect3DTexture8*	mp_texture;
	int					m_time_removed;
};



// Here's a machine specific implementation of the CImposterGroup
class CXboxImposterGroup : public CImposterGroup
{
	public:
												CXboxImposterGroup();
	virtual										~CXboxImposterGroup();


	private:									// It's all private, as it is machine specific

	virtual void								plat_create_imposter_polygon( void );
	virtual void								plat_remove_imposter_polygon( void );
	virtual bool								plat_update_imposter_polygon( void );
	virtual void								plat_draw_imposter_polygon( void );
	virtual float								plat_check_distance( void );
	virtual void								plat_process( void );

	void										process_removed_textures( void );

	// Machine specific members
	sImposterPolyVert							m_vertex_buffer[4];
	IDirect3DTexture8*							mp_texture;
	Lst::Head <sRemovedTextureDetails> *		mp_removed_textures_list;
};

} // Namespace Nx  			

#endif
