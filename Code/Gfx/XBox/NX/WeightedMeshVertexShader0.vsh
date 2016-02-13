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

    ; Scale transformed point by weight
    mul     VSTMP_REG_POS_ACCUM.xyz, VSTMP_REG_POS_TMP.xyz, VSIN_REG_WEIGHTS.x

    ; Transform normal
    dp3		VSTMP_REG_NORMAL_TMP.x, VSIN_REG_NORMAL, c[0 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp3		VSTMP_REG_NORMAL_TMP.y, VSIN_REG_NORMAL, c[1 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp3		VSTMP_REG_NORMAL_TMP.z, VSIN_REG_NORMAL, c[2 + a0.x VSCONST_REG_MATRIX_OFFSET]

    ; Scale transformed normal by weight
    mul     VSTMP_REG_NORMAL_ACCUM.xyz, VSTMP_REG_NORMAL_TMP.xyz, VSIN_REG_WEIGHTS.x

;------------------------------------------------------------------------------

    ; Get matrix index 2
    mov     a0.x, VSIN_REG_INDICES.y

    ; Transform position
    dp4		VSTMP_REG_POS_TMP.x, VSIN_REG_POS, c[0 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.y, VSIN_REG_POS, c[1 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.z, VSIN_REG_POS, c[2 + a0.x VSCONST_REG_MATRIX_OFFSET]

    ; Scale transformed point by weight and add to previous
    mad     VSTMP_REG_POS_ACCUM.xyz, VSTMP_REG_POS_TMP.xyz, VSIN_REG_WEIGHTS.y, VSTMP_REG_POS_ACCUM.xyz

    ; Transform normal
    dp3		VSTMP_REG_NORMAL_TMP.x, VSIN_REG_NORMAL, c[0 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp3		VSTMP_REG_NORMAL_TMP.y, VSIN_REG_NORMAL, c[1 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp3		VSTMP_REG_NORMAL_TMP.z, VSIN_REG_NORMAL, c[2 + a0.x VSCONST_REG_MATRIX_OFFSET]

    ; Scale transformed point by weight
    mad     VSTMP_REG_NORMAL_ACCUM.xyz, VSTMP_REG_NORMAL_TMP.xyz, VSIN_REG_WEIGHTS.y, VSTMP_REG_NORMAL_ACCUM.xyz

;------------------------------------------------------------------------------

    ; Get matrix index 3
    mov     a0.x, VSIN_REG_INDICES.z

    ; Transform position
    dp4		VSTMP_REG_POS_TMP.x, VSIN_REG_POS, c[0 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.y, VSIN_REG_POS, c[1 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp4		VSTMP_REG_POS_TMP.z, VSIN_REG_POS, c[2 + a0.x VSCONST_REG_MATRIX_OFFSET]

    ; Scale transformed point by weight and add to previous
    mad     VSTMP_REG_POS_ACCUM.xyz, VSTMP_REG_POS_TMP.xyz, VSIN_REG_WEIGHTS.z, VSTMP_REG_POS_ACCUM.xyz

    ; Transform normal
    dp3		VSTMP_REG_NORMAL_TMP.x, VSIN_REG_NORMAL, c[0 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp3		VSTMP_REG_NORMAL_TMP.y, VSIN_REG_NORMAL, c[1 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp3		VSTMP_REG_NORMAL_TMP.z, VSIN_REG_NORMAL, c[2 + a0.x VSCONST_REG_MATRIX_OFFSET]

    ; Scale transformed point by weight
    mad     VSTMP_REG_NORMAL_ACCUM.xyz, VSTMP_REG_NORMAL_TMP.xyz, VSIN_REG_WEIGHTS.z, VSTMP_REG_NORMAL_ACCUM.xyz

    ; Copy w
    mov		VSTMP_REG_POS_ACCUM.w, VSIN_REG_POS.w

;------------------------------------------------------------------------------
; Combined camera & projection matrix
;------------------------------------------------------------------------------

    dp4		oPos.x, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_X
    dp4		oPos.y, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Y
    dp4		oPos.z, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_Z
    dp4     oPos.w, VSTMP_REG_POS_ACCUM, VSCONST_REG_TRANSFORM_W

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

;------------------------------------------------------------------------------
; oPos to screenspace transformation
;------------------------------------------------------------------------------

	mul		oPos.xyz, r12, c94			; scale
	+ rcc	r1.x, r12.w					; compute 1/w
	mad		oPos.xyz, r12, r1.x, c95	; scale by 1/w, add offset
