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

	; Transform normal by weighted matrix
    dp3     VSTMP_REG_NORMAL_ACCUM.x, VSIN_REG_NORMAL, VSTMP_REG_MAT0
	dp3		VSTMP_REG_NORMAL_ACCUM.y, VSIN_REG_NORMAL, VSTMP_REG_MAT1
	dp3		VSTMP_REG_NORMAL_ACCUM.z, VSIN_REG_NORMAL, VSTMP_REG_MAT2

;------------------------------------------------------------------------------
; Combined camera & projection matrix
;------------------------------------------------------------------------------

    dph		oPos.x, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_X
    dph		oPos.y, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Y
    dph		oPos.z, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Z
    dph     oPos.w, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_W

;------------------------------------------------------------------------------
; Deal with fog value (r12 shadows oPos)...
;------------------------------------------------------------------------------

	mov		oFog.x, r12.w

;------------------------------------------------------------------------------
; Transform normal by combined camera & projection matrix
;------------------------------------------------------------------------------

    dp3		VSTMP_REG_NORMAL_TMP.x, VSTMP_REG_NORMAL_ACCUM, VSCONST_REG_WORLD_TRANSFORM_X
    dp3		VSTMP_REG_NORMAL_TMP.y, VSTMP_REG_NORMAL_ACCUM, VSCONST_REG_WORLD_TRANSFORM_Y
    dp3		VSTMP_REG_NORMAL_TMP.z, VSTMP_REG_NORMAL_ACCUM, VSCONST_REG_WORLD_TRANSFORM_Z

;------------------------------------------------------------------------------
; Single directional light + Ambient
;------------------------------------------------------------------------------

    ; Normalize
    dp3     VSTMP_REG_NORMAL_TMP.w, VSTMP_REG_NORMAL_TMP, VSTMP_REG_NORMAL_TMP
    rsq     VSTMP_REG_NORMAL_TMP.w, VSTMP_REG_NORMAL_TMP.w
    mul     VSTMP_REG_NORMAL_TMP.xyz, VSTMP_REG_NORMAL_TMP.xyz, VSTMP_REG_NORMAL_TMP.w

    ; DP normal & light0 dir clamp then scale by light color
    dp3     VSTMP_REG_NORMAL_TMP.w, VSTMP_REG_NORMAL_TMP, VSCONST_REG_LIGHT_DIR0
    max     VSTMP_REG_NORMAL_TMP.w, VSCONST_REG_LIGHT_DIR0.w, -VSTMP_REG_NORMAL_TMP.w

	; This is where the ambient gets added in.
    mov     r11, VSCONST_REG_AMB_LIGHT_COLOR

    mad     r11, VSCONST_REG_LIGHT_COLOR0, VSTMP_REG_NORMAL_TMP.w, r11

    ; DP normal & light1 dir clamp then scale by light color
    dp3     VSTMP_REG_NORMAL_TMP.w, VSTMP_REG_NORMAL_TMP, VSCONST_REG_LIGHT_DIR1
    max     VSTMP_REG_NORMAL_TMP.w, VSCONST_REG_LIGHT_DIR1.w, -VSTMP_REG_NORMAL_TMP.w
    mad     r11, VSCONST_REG_LIGHT_COLOR1, VSTMP_REG_NORMAL_TMP.w, r11

    ; DP normal & light2 dir clamp then scale by light color
    dp3     VSTMP_REG_NORMAL_TMP.w, VSTMP_REG_NORMAL_TMP, VSCONST_REG_LIGHT_DIR2
    max     VSTMP_REG_NORMAL_TMP.w, VSCONST_REG_LIGHT_DIR2.w, -VSTMP_REG_NORMAL_TMP.w
;    mad     oD0, VSCONST_REG_LIGHT_COLOR2, VSTMP_REG_NORMAL_TMP.w, r11
    mad     r11, VSCONST_REG_LIGHT_COLOR2, VSTMP_REG_NORMAL_TMP.w, r11

	; Material color attenuation
	mul		oD0, r11, VSCONST_REG_MATERIAL_COLOR

;------------------------------------------------------------------------------
; Copy texture coordinates
;------------------------------------------------------------------------------

    mov     oT0, VSIN_REG_TEXCOORDS0
    mov     oT1, VSIN_REG_TEXCOORDS1
    mov     oT2, VSIN_REG_TEXCOORDS2

;------------------------------------------------------------------------------
; oPos to screenspace transformation
;------------------------------------------------------------------------------

	mul		oPos.xyz, r12, c94			; scale
	+ rcc	r1.x, r12.w					; compute 1/w
	mad		oPos.xyz, r12, r1.x, c95	; scale by 1/w, add offset
