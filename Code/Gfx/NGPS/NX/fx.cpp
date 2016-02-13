#include <core\defines.h>
#include "math.h"
#include "fx.h"
#include "asmdma.h"


namespace NxPs2
{



void Fx::SetupFogPalette(uint32 FogRGBA, float Exponent)
{
	int i,j;
	float t, alpha = (float)(int)(FogRGBA>>24);

	for (i=255,t=1.0f/256.0f; i>=0; i--,t+=1.0f/256.0f)
	{
		j = (i&0xE7) | (i&0x10)>>1 | (i&0x08)<<1;
		FogPalette[j] = (FogRGBA & 0x00FFFFFF) | (uint32)(int)(alpha * powf(t,Exponent) + 0.5f)<<24;
	}
}



} // namespace NxPs2

