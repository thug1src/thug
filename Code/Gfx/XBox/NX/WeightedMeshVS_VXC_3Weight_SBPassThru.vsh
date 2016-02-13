xvs.1.1

#include "anim_vertdefs.h"

#pragma screenspace

;------------------------------------------------------------------------------
; Bone space transforms
;------------------------------------------------------------------------------

    ; Get matrix index 1
    mov     a0.x, VSIN_REG_INDICES.x

	; Weight matrix
	mul		VSTMP_REG_MAT0, VSIN_REG_WEIGHTS.x, c[0 + a0.x VSCONST_REG_MATRIX_OFFSET]
	mul		VSTMP_REG_MAT1, VSIN_REG_WEIGHTS.x, c[1 + a0.x VSCONST_REG_MATRIX_OFFSET]
	mul		VSTMP_REG_MAT2, VSIN_REG_WEIGHTS.x, c[2 + a0.x VSCONST_REG_MATRIX_OFFSET]

;------------------------------------------------------------------------------

    ; Get matrix index 2
    mov     a0.x, VSIN_REG_INDICES.y

	; Weight matrix
	mad		VSTMP_REG_MAT0, VSIN_REG_WEIGHTS.y, c[0 + a0.x VSCONST_REG_MATRIX_OFFSET], VSTMP_REG_MAT0
	mad		VSTMP_REG_MAT1, VSIN_REG_WEIGHTS.y, c[1 + a0.x VSCONST_REG_MATRIX_OFFSET], VSTMP_REG_MAT1
	mad		VSTMP_REG_MAT2, VSIN_REG_WEIGHTS.y, c[2 + a0.x VSCONST_REG_MATRIX_OFFSET], VSTMP_REG_MAT2

;------------------------------------------------------------------------------

    ; Get matrix index 3
    mov     a0.x, VSIN_REG_INDICES.z

	; Weight matrix
	mad		VSTMP_REG_MAT0, VSIN_REG_WEIGHTS.z, c[0 + a0.x VSCONST_REG_MATRIX_OFFSET], VSTMP_REG_MAT0
	mad		VSTMP_REG_MAT1, VSIN_REG_WEIGHTS.z, c[1 + a0.x VSCONST_REG_MATRIX_OFFSET], VSTMP_REG_MAT1
	mad		VSTMP_REG_MAT2, VSIN_REG_WEIGHTS.z, c[2 + a0.x VSCONST_REG_MATRIX_OFFSET], VSTMP_REG_MAT2

	; Transform position by weighted matrix
    dp4     VSTMP_REG_POS_ACCUM.x, VSIN_REG_POS, VSTMP_REG_MAT0
	dp4		VSTMP_REG_POS_ACCUM.y, VSIN_REG_POS, VSTMP_REG_MAT1
	dp4		VSTMP_REG_POS_ACCUM.z, VSIN_REG_POS, VSTMP_REG_MAT2

    ; Copy w
    mov		VSTMP_REG_POS_ACCUM.w, VSIN_REG_POS.w
    
;------------------------------------------------------------------------------
; Combined camera & projection matrix
;------------------------------------------------------------------------------

    dph		oPos.x, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_X
    dph		oPos.y, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Y
    dph		oPos.z, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Z
    dph     oPos.w, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_W

;------------------------------------------------------------------------------
; Calculate second set of texture coordinates (into 4th slot) by transforming the world space
; position into the shadowbuffer space
;------------------------------------------------------------------------------

	dp4		oT3.x, VSTMP_REG_POS_ACCUM,	VSCONST_REG_SHADOWBUFFER_TRANSFORM_X
	dp4		oT3.y, VSTMP_REG_POS_ACCUM, VSCONST_REG_SHADOWBUFFER_TRANSFORM_Y
	dp4		oT3.z, VSTMP_REG_POS_ACCUM, VSCONST_REG_SHADOWBUFFER_TRANSFORM_Z
	dp4		r0.w,  VSTMP_REG_POS_ACCUM, VSCONST_REG_SHADOWBUFFER_TRANSFORM_W

	;clamp w (q) to 0
	slt		r1, c0, c0
	max		r0.w, r0.w, r1.w
	mov		oT3.w, r0.w

;------------------------------------------------------------------------------
; oPos to screenspace transformation
;------------------------------------------------------------------------------

	mul		oPos.xyz, r12, c94			; scale
	+ rcc	r1.x, r12.w					; compute 1/w
	mad		oPos.xyz, r12, r1.x, c95	; scale by 1/w, add offset
