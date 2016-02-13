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
; Multiple directional lights plus ambient, plus transform texture coordinates
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

	dp4     oT0.x, VSIN_REG_TEXCOORDS0, VSCONST_REG_UV_MAT00								; u' = ( s * mat00.x ) + ( t * mat00.y ) + ( 1.0 * mat00.w )
	dp4     oT0.y, VSIN_REG_TEXCOORDS0, VSCONST_REG_UV_MAT01								; v' = ( s * mat01.x ) + ( t * mat01.y ) + ( 1.0 * mat01.w )
	dp4     oT1.x, VSIN_REG_TEXCOORDS1, VSCONST_REG_UV_MAT10								; u' = ( s * mat10.x ) + ( t * mat10.y ) + ( 1.0 * mat10.w )
	dp4     oT1.y, VSIN_REG_TEXCOORDS1, VSCONST_REG_UV_MAT11								; v' = ( s * mat11.x ) + ( t * mat11.y ) + ( 1.0 * mat11.w )
	dp4     oT2.x, VSIN_REG_TEXCOORDS2, VSCONST_REG_UV_MAT20								; u' = ( s * mat20.x ) + ( t * mat20.y ) + ( 1.0 * mat20.w )
	dp4     oT2.y, VSIN_REG_TEXCOORDS2, VSCONST_REG_UV_MAT21								; v' = ( s * mat21.x ) + ( t * mat21.y ) + ( 1.0 * mat21.w )
	dp4     oT3.x, VSIN_REG_TEXCOORDS3, VSCONST_REG_UV_MAT30								; u' = ( s * mat30.x ) + ( t * mat30.y ) + ( 1.0 * mat30.w )
	dp4     oT3.y, VSIN_REG_TEXCOORDS3, VSCONST_REG_UV_MAT31								; v' = ( s * mat31.x ) + ( t * mat31.y ) + ( 1.0 * mat31.w )

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
