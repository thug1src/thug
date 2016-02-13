#ifndef __MICROCODE_H
#define __MICROCODE_H

// the __attribute__ things are there so the code can compile using GP relative
// otherwise, the compiler assumes these go in the .lit4 section, and we get a link error, as they don't

#ifdef __PLAT_NGPS__

extern uint MPGStart       __attribute__((section(".vudata")));
extern uint MPGEnd         __attribute__((section(".vudata")));
extern uint Setup          __attribute__((section(".vudata")));
extern uint Jump           __attribute__((section(".vudata")));
extern uint Breakpoint     __attribute__((section(".vudata")));
extern uint ParseInit      __attribute__((section(".vudata")));
extern uint Parser         __attribute__((section(".vudata")));
extern uint L_VF09         __attribute__((section(".vudata")));
extern uint L_VF10         __attribute__((section(".vudata")));
extern uint L_VF11         __attribute__((section(".vudata")));
extern uint L_VF12         __attribute__((section(".vudata")));
extern uint L_VF13         __attribute__((section(".vudata")));
extern uint L_VF14         __attribute__((section(".vudata")));
extern uint L_VF15         __attribute__((section(".vudata")));
extern uint L_VF16         __attribute__((section(".vudata")));
extern uint L_VF17         __attribute__((section(".vudata")));
extern uint L_VF18         __attribute__((section(".vudata")));
extern uint L_VF19         __attribute__((section(".vudata")));
extern uint L_VF20         __attribute__((section(".vudata")));
extern uint L_VF21         __attribute__((section(".vudata")));
extern uint L_VF22         __attribute__((section(".vudata")));
extern uint L_VF23         __attribute__((section(".vudata")));
extern uint L_VF24         __attribute__((section(".vudata")));
extern uint L_VF25         __attribute__((section(".vudata")));
extern uint L_VF26         __attribute__((section(".vudata")));
extern uint L_VF27         __attribute__((section(".vudata")));
extern uint L_VF28         __attribute__((section(".vudata")));
extern uint L_VF39         __attribute__((section(".vudata")));
extern uint L_VF30         __attribute__((section(".vudata")));
extern uint L_VF31         __attribute__((section(".vudata")));
extern uint GSPrim         __attribute__((section(".vudata")));
extern uint Proj           __attribute__((section(".vudata")));
extern uint PTex           __attribute__((section(".vudata")));
extern uint Refl           __attribute__((section(".vudata")));
extern uint Line           __attribute__((section(".vudata")));
extern uint Skin           __attribute__((section(".vudata")));
extern uint Sprites        __attribute__((section(".vudata")));
extern uint SpriteCull     __attribute__((section(".vudata")));
extern uint ReformatXforms __attribute__((section(".vudata")));
extern uint ShadowVolumeSkin __attribute__((section(".vudata")));

#endif

#ifdef __PLAT_WN32__

//#define Setup		 0
//#define Jump		 2
#define Breakpoint	 4
#define ParseInit	 6
#define Parser		 8
#define L_VF09		10	// (Near, Far, k/(xRes/2), k/(yRes/2)) where k=viewport_scale_x, should be 2048 but is 1900 because of clipper problem
#define L_VF10		12	// inverse viewport scale vector
#define L_VF11		14	// inverse viewport offset vector
#define L_VF12		16	// row 0, local to viewport transform
#define L_VF13		18	// row 1, local to viewport transform
#define L_VF14		20	// row 2, local to viewport transform
#define L_VF15		22	// row 3, local to viewport transform
#define L_VF16		24	// lightsource 2 colour (r,g,b,?)
#define L_VF17		26	// row 0, reflection map transform
#define L_VF18		28	// row 1, reflection map transform
#define L_VF19		30	// row 2, reflection map transform
#define L_VF20		32	// light vectors, x components
#define L_VF21		34	// light vectors, y components
#define L_VF22		36	// light vectors, z components
#define L_VF23		38	// ambient colour (r,g,b,?)
#define L_VF24		40	// lightsource 0 colour (r,g,b,?)
#define L_VF25		42	// lightsource 1 colour (r,g,b,?)
#define L_VF26		44	// texture projection scale vector
#define L_VF27		46	// texture projection offset vector
#define L_VF28		48	// saves the z-components of the view matrix during a z-push
#define L_VF29		50	// temp skinning
#define L_VF30		52	// temp skinning
#define L_VF31		54	// temp skinning
#define GSPrim		56
#define Proj		58
#define PTex		60
#define Refl		62
#define Line		64
#define Skin		66
#define Light		68
#define LightT		70
#define WibbleT		72
#define LWibT		74
#define AddZPush	76
#define SubZPush	78
#define Setup		80
#define Jump		82
#define SCAB		84
#define LAB			86
#define SHAB		88

#endif

#define PROJ 0x00
#define CULL 0x01		// per-triangle view culling
#define CLIP 0x02		// full 3D clipping of triangles
#define SHDW 0x04		// skinned=>cast shadow into texture; non-skinned=>render mesh with projected shadow texture on it
#define COLR 0x08		// apply colour at vertices
#define FOGE 0x10		// calculate per-vertex fog coefficient
#define WIRE 0x20		// render skinned as wireframe (but doesn't render all edges)

#endif //__MICROCODE_H

