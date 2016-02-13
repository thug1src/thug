#ifndef __IMPORT_H
#define __IMPORT_H

#include <core/HashTable.h>
#include "texture.h"
#include "gfx/nxtexture.h"

namespace NxNgc
{

void LoadTextureFile( const char* Filename, Lst::HashTable<Nx::CTexture> * p_texture_table );

} // namespace NxNgc


#endif // __IMPORT_H

