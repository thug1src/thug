#ifndef __SPRITE_H
#define __SPRITE_H

#include "texture.h"

namespace NxNgc
{

struct SDraw2D
{
					SDraw2D( float pri = 0.0f, bool hide = true );
	virtual			~SDraw2D( void );

	void			SetPriority( float pri );
	float			GetPriority( void ) const;

	void			SetHidden( bool hide );
	bool			IsHidden( void ) const;

	// members
	SDraw2D			*mp_next;

	// Statics
	static void		DrawAll( void );

private:
	void			InsertDrawList( void );
	void			RemoveDrawList( void );

	virtual void	BeginDraw( void ) = 0;
	virtual void	Draw( void ) = 0;
	virtual void	EndDraw( void ) = 0;

	// Not even the derived classes should have direct access
	bool			m_hidden;
	float			m_pri;

	// 2D draw list (sorted by priority);
	static SDraw2D	*sp_2D_draw_list;
};


struct sSprite : public SDraw2D
{
	public:
					sSprite( float pri = 0.0f );
					~sSprite();

	sTexture		*mp_texture;

	float			m_xpos;
	float			m_ypos;
	uint16			m_width;
	uint16			m_height;
	float			m_scale_x;
	float			m_scale_y;
	float			m_xhot;
	float			m_yhot;
	float			m_rot;
	uint32			m_rgba;

private:

//	IDirect3DVertexBuffer8	*p_vertex_buffer;
	
	void					BeginDraw();
	void					Draw();
	void					EndDraw(void);
};


} // namespace NxNgc


#endif // __SPRITE_H

