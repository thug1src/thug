xvs.1.1

#include "anim_vertdefs.h"

#pragma screenspace

;------------------------------------------------------------------------------
; Bone space transforms
;------------------------------------------------------------------------------

    ; Get matrix index 1
    mov     a0.x, VSIN_REG_INDICES.x

    ; Transform position
    dp4		VSTMP_REG_POS_TMP.x, VSIN_REG_POS, c[0 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.y, VSIN_REG_POS, c[1 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.z, VSIN_REG_POS, c[2 + a0.x VSCONST_REG_MATRIX_OFFSET]
    mov		VSTMP_REG_POS_TMP.w, VSIN_REG_POS.w

	; No need to scale by results by vertex weights, since this is a one-weight shader so the weight
	; is assumed to be 1.0

;------------------------------------------------------------------------------
; Combined camera & projection matrix
;------------------------------------------------------------------------------

    dp4		oPos.x, VSTMP_REG_POS_TMP, VSCONST_REG_TRANSFORM_X
    dp4		oPos.y, VSTMP_REG_POS_TMP, VSCONST_REG_TRANSFORM_Y
    dp4		oPos.z, VSTMP_REG_POS_TMP, VSCONST_REG_TRANSFORM_Z
    dp4     oPos.w, VSTMP_REG_POS_TMP, VSCONST_REG_TRANSFORM_W

;------------------------------------------------------------------------------
; Calculate second set of texture coordinates (into 4th slot) by transforming the world space
; position into the shadowbuffer space
;------------------------------------------------------------------------------

	dp4		oT3.x, VSTMP_REG_POS_TMP, VSCONST_REG_SHADOWBUFFER_TRANSFORM_X
	dp4		oT3.y, VSTMP_REG_POS_TMP, VSCONST_REG_SHADOWBUFFER_TRANSFORM_Y
	dp4		oT3.z, VSTMP_REG_POS_TMP, VSCONST_REG_SHADOWBUFFER_TRANSFORM_Z
	dp4		r0.w,  VSTMP_REG_POS_TMP, VSCONST_REG_SHADOWBUFFER_TRANSFORM_W

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
