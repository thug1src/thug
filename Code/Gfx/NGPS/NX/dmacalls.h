#ifndef __MODELS_H
#define __MODELS_H


namespace NxPs2
{

enum eDmaSubroutines
{
	CLEAR_VRAM,
	FLIP_N_CLEAR,
	CLEAR_SCREEN,
	SET_RENDERSTATE,
	CLEAR_ZBUFFER,
	UPLOAD_TEXTURES,
	SET_FOGCOL,
	FLIP_N_CLEAR_LETTERBOX,

	NUM_SUBROUTINES
};


uint8 *BeginDmaSubroutine(void);
void EndDmaSubroutine(void);
void BuildDmaSubroutines(void);

//-------------------------
//		G L O B A L S
//-------------------------

extern uint8 *pSubroutine;

} // namespace NxPs2


#endif // __MODELS_H

