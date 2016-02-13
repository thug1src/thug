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

;------------------------------------------------------------------------------

    ; Get matrix index 4
    mov     a0.x, VSIN_REG_INDICES.w

	; Weight matrix
	mad		VSTMP_REG_MAT0, VSIN_REG_WEIGHTS.w, c[0 + a0.x VSCONST_REG_MATRIX_OFFSET], VSTMP_REG_MAT0
	mad		VSTMP_REG_MAT1, VSIN_REG_WEIGHTS.w, c[1 + a0.x VSCONST_REG_MATRIX_OFFSET], VSTMP_REG_MAT1
	mad		VSTMP_REG_MAT2, VSIN_REG_WEIGHTS.w, c[2 + a0.x VSCONST_REG_MATRIX_OFFSET], VSTMP_REG_MAT2

	; Transform position by weighted matrix
    dp4     VSTMP_REG_POS_ACCUM.x, VSIN_REG_POS, VSTMP_REG_MAT0
	dp4		VSTMP_REG_POS_ACCUM.y, VSIN_REG_POS, VSTMP_REG_MAT1
	dp4		VSTMP_REG_POS_ACCUM.z, VSIN_REG_POS, VSTMP_REG_MAT2

	; Transform normal by weighted matrix
	dp3     VSTMP_REG_NORMAL_TMP.x, VSIN_REG_NORMAL, VSTMP_REG_MAT0
	dp3		VSTMP_REG_NORMAL_TMP.y, VSIN_REG_NORMAL, VSTMP_REG_MAT1
	dp3		VSTMP_REG_NORMAL_TMP.z, VSIN_REG_NORMAL, VSTMP_REG_MAT2

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

	mov		oFog.x, -r12.w

;------------------------------------------------------------------------------
; Multiple directional lights plus ambient
;------------------------------------------------------------------------------

    ; DP normal & light0 dir clamp then scale by light color
    dp3     VSTMP_REG_NORMAL_TMP.w, VSTMP_REG_NORMAL_TMP, VSCONST_REG_LIGHT_DIR0
    max     VSTMP_REG_NORMAL_TMP.w, VSCONST_REG_LIGHT_DIR0.w, -VSTMP_REG_NORMAL_TMP.w

	; For specular calculations further down, save this result off into r0.x
	mov		r0.x, VSTMP_REG_NORMAL_TMP.w

	; This is where the ambient gets added in.
    mov     r11, VSCONST_REG_AMB_LIGHT_COLOR

    mad     r11, VSCONST_REG_LIGHT_COLOR0, VSTMP_REG_NORMAL_TMP.w, r11

    ; DP normal & light1 dir clamp then scale by light color
    dp3     VSTMP_REG_NORMAL_TMP.w, VSTMP_REG_NORMAL_TMP, VSCONST_REG_LIGHT_DIR1
    max     VSTMP_REG_NORMAL_TMP.w, VSCONST_REG_LIGHT_DIR1.w, -VSTMP_REG_NORMAL_TMP.w
    mad     r11, VSCONST_REG_LIGHT_COLOR1, VSTMP_REG_NORMAL_TMP.w, r11

    ; DP normal & light2 dir clamp then scale by light color (third light currently deactivated).
;	dp3     VSTMP_REG_NORMAL_TMP.w, VSTMP_REG_NORMAL_TMP, VSCONST_REG_LIGHT_DIR2
;	max     VSTMP_REG_NORMAL_TMP.w, VSCONST_REG_LIGHT_DIR2.w, -VSTMP_REG_NORMAL_TMP.w
;	mad     r11, VSCONST_REG_LIGHT_COLOR2, VSTMP_REG_NORMAL_TMP.w, r11

	; Vertex color attenuation
	mul		oD0, r11, VSIN_REG_COLOR

;------------------------------------------------------------------------------
; Specular calculation
;------------------------------------------------------------------------------

	; Calculate vector to eye (V)
	sub		r7.xyz, VSCONST_REG_CAM_POS, VSTMP_REG_POS_ACCUM
	
	; Normalize V
	dp3		r7.w, r7.xyz, r7.xyz
	rsq		r1.x, r7.w
	mul		r7, r7, r1.x

	; Calculate H = L + V
	add		r7, -VSCONST_REG_LIGHT_DIR0, r7

	; Normalize H
	dp3		r7.w, r7.xyz, r7.xyz
	rsq		r1.x, r7.w
	mul		r7, r7, r1.x

	; Calculate N.H (don't worry about clamping to zero, since the lit instruction below does this)
	dp3		r0.y, VSTMP_REG_NORMAL_TMP, r7

	; Move the power term over into r0
	mov		r0.w, VSCONST_REG_SPECULAR_COLOR.w
	
	; Specular lighting calc - (N.H)^pow
	lit		r1.z, r0

	; Modulate specular color by specular result.
;	mov		oD1.xyz, r1.z
	mul		oD1.xyz, r1.z, VSCONST_REG_SPECULAR_COLOR.xyz

;------------------------------------------------------------------------------
; Copy texture coordinates
;------------------------------------------------------------------------------

    mov     oT0, VSIN_REG_TEXCOORDS0
    mov     oT1, VSIN_REG_TEXCOORDS1
    mov     oT2, VSIN_REG_TEXCOORDS2
    mov     oT3, VSIN_REG_TEXCOORDS3

;------------------------------------------------------------------------------
; oPos to screenspace transformation
;------------------------------------------------------------------------------

	mul		oPos.xyz, r12, c94			; scale
	+ rcc	r1.x, r12.w					; compute 1/w
	mad		oPos.xyz, r12, r1.x, c95	; scale by 1/w, add offset
