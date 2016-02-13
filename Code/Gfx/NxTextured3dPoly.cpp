#include <core/defines.h>
#include <core/math.h>

#include <gfx/nxtextured3dpoly.h>
#include <core/crc.h>
#include <core/hashtable.h>

namespace Nx
{

static Lst::HashTable< Nx::CTextured3dPoly > s_poly_table(1);

void CTextured3dPoly::sRenderAll()
{
	CTextured3dPoly *p_poly;
	s_poly_table.IterateStart();
	while(( p_poly = s_poly_table.IterateNext()))
	{
		p_poly->Render();
	}
}

CTextured3dPoly::CTextured3dPoly()
{
	s_poly_table.PutItem((uint32)this,this);
}

CTextured3dPoly::~CTextured3dPoly()
{
	s_poly_table.FlushItem((uint32)this);
}

void CTextured3dPoly::SetTexture(uint32 texture_checksum)
{
	plat_set_texture(texture_checksum);
}

void CTextured3dPoly::SetTexture(const char *p_textureName)
{
	SetTexture(Crc::GenerateCRCFromString(p_textureName));
}
	
void CTextured3dPoly::SetPos(const Mth::Vector &pos, float width, float height, const Mth::Vector &normal, float angle)
{
	Mth::Vector offx(1.0f,0.0f,0.0f);
	Mth::Vector offz=Mth::CrossProduct(normal,offx);
	
	offx*=width/2.0f;
	offz*=height/2.0f;
	
	mp_pos[0]=pos-offx-offz;
	mp_pos[1]=pos+offx-offz;
	mp_pos[2]=pos+offx+offz;
	mp_pos[3]=pos-offx+offz;
}

void CTextured3dPoly::Render()
{
	plat_render();
}	

} // namespace Nx

