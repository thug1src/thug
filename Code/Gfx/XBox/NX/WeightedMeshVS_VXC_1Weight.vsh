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

    ; Transform normal
    dp3		VSTMP_REG_NORMAL_TMP.x, VSIN_REG_NORMAL, c[0 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp3		VSTMP_REG_NORMAL_TMP.y, VSIN_REG_NORMAL, c[1 + a0.x VSCONST_REG_MATRIX_OFFSET]
    dp3		VSTMP_REG_NORMAL_TMP.z, VSIN_REG_NORMAL, c[2 + a0.x VSCONST_REG_MATRIX_OFFSET]

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
; Multiple directional lights plus ambient, plus copy texture coordinates
; (interleaving these helps prevent stalls)
;------------------------------------------------------------------------------

    dp3		VSTMP_REG_NORMAL_ACCUM.x, VSTMP_REG_NORMAL_TMP, VSCONST_REG_LIGHT_DIR0			; Light0 dir.normal
    dp3		VSTMP_REG_NORMAL_ACCUM.y, VSTMP_REG_NORMAL_TMP, VSCONST_REG_LIGHT_DIR1			; Light1 dir.normal
	dp3     VSTMP_REG_NORMAL_ACCUM.z, VSTMP_REG_NORMAL_TMP, VSCONST_REG_LIGHT_DIR2			; Light2 dir.normal

	max     VSTMP_REG_NORMAL_ACCUM.x, VSCONST_REG_LIGHT_DIR0.w, -VSTMP_REG_NORMAL_ACCUM.x	; Light0 result clamp
    max     VSTMP_REG_NORMAL_ACCUM.y, VSCONST_REG_LIGHT_DIR1.w, -VSTMP_REG_NORMAL_ACCUM.y	; Light1 result clamp
	max     VSTMP_REG_NORMAL_ACCUM.z, VSCONST_REG_LIGHT_DIR2.w, -VSTMP_REG_NORMAL_ACCUM.z	; Light2 result clamp

    mov     r11, VSCONST_REG_AMB_LIGHT_COLOR												; Accumulate start with ambient
    mad     r11, VSCONST_REG_LIGHT_COLOR0, VSTMP_REG_NORMAL_ACCUM.x, r11					; Accumulate Light0 result modulated with Light0 color
    mad     r11, VSCONST_REG_LIGHT_COLOR1, VSTMP_REG_NORMAL_ACCUM.y, r11					; Accumulate Light1 result modulated with Light1 color
	mad     r11, VSCONST_REG_LIGHT_COLOR2, VSTMP_REG_NORMAL_ACCUM.z, r11					; Accumulate Light2 result modulated with Light2 color

	mov     oT0, VSIN_REG_TEXCOORDS0
	mov     oT1, VSIN_REG_TEXCOORDS1
	mov     oT2, VSIN_REG_TEXCOORDS2
	mov     oT3, VSIN_REG_TEXCOORDS3

	mul		oD0, r11, VSIN_REG_COLOR														; Vertex color attenuation

;------------------------------------------------------------------------------
; Deal with fog value (r12 shadows oPos)...
;------------------------------------------------------------------------------

	mov		oFog.x, -r12.w

;------------------------------------------------------------------------------
; oPos to screenspace transformation
;------------------------------------------------------------------------------

	mul		oPos.xyz, r12, c94			; scale
	+ rcc	r1.x, r12.w					; compute 1/w
	mad		oPos.xyz, r12, r1.x, c95	; scale by 1/w, add offset
