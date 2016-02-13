#include <Gfx/NGPS/p_NxTextured3dPoly.h>
#include <Gfx/NGPS/p_nxtexture.h>
#include <Gfx/NGPS/nx/immediate.h>
#include <Gfx/NGPS/nx/gs.h>
#include <Gfx/NGPS/nx/vif.h>
#include <gfx/nxtexman.h>

namespace NxPs2
{

CPs2Textured3dPoly::CPs2Textured3dPoly()
{
}

CPs2Textured3dPoly::~CPs2Textured3dPoly()
{
}

void CPs2Textured3dPoly::plat_set_texture(uint32 texture_checksum)
{
	Nx::CTexture *p_texture = Nx::CTexDictManager::sp_sprite_tex_dict->GetTexture(texture_checksum);
	Dbg_MsgAssert(p_texture, ("no texture found for sprite"));
	Nx::CPs2Texture *p_ps2_texture = static_cast<Nx::CPs2Texture*>( p_texture );
	Dbg_MsgAssert(p_ps2_texture,("NULL p_ps2_texture"));
	mp_engine_texture = p_ps2_texture->GetSingleTexture(); 
	Dbg_MsgAssert(mp_engine_texture,("NULL mp_engine_texture"));
}	

void CPs2Textured3dPoly::plat_render()
{
	CImmediateMode::sStartPolyDraw( mp_engine_texture, PackALPHA(0,0,0,0,0), ABS );
	
	CImmediateMode::sDrawQuadTexture(mp_engine_texture, mp_pos[0], mp_pos[1], mp_pos[2], mp_pos[3], 0x80808080,0x80808080,0x80808080,0x80808080);
}


} // Namespace NxPs2
				
				
