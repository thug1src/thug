xvs.1.1

; Constants:
;  c0 - c3		WVP Matrix (WORLD*VIEW*PROJECTION)
;  c4			Screen right vector
;  c5			Screen up vector
;  c6			Screen at vector

; In:
;   v0 - Position		(actually the position of the pivot point)
;	v1 - Normal			(actually the position of the point relative to the pivot)
;   v2 - Vertex color
;   v3 - TexCoord0
;   v4 - TexCoord1
;   v5 - TexCoord2
;   v6 - TexCoord3

; Out:
;   oPos - Position
;   oTn - TextureCoords

;------------------------------------------------------------------------------
; Pivot relative position -> world relative position, plus copy texture coordinates
; (interleaving these helps prevent stalls)
;------------------------------------------------------------------------------
mul	r0.xyz,	v1.x, c4
mov oT0,	v3
mad r0.xyz, v1.y, c5, r0
mov oT1,	v4
mad r0.xyz,	v1.z, c6, r0
mov oT2,	v5
add r0.xyz,	v0, r0
mov oT3,	v6
sge r0.w,	c0.x, c0.x		; Set r0.w = 1.0

;------------------------------------------------------------------------------
; Vertex color
;------------------------------------------------------------------------------
mov oD0, v2

;------------------------------------------------------------------------------
; Vertex->screen transform
;------------------------------------------------------------------------------
dp4 oPos.x, r0, c0
dp4 oPos.y, r0, c1
dp4 oPos.z, r0, c2
dp4 oPos.w, r0, c3

;------------------------------------------------------------------------------
; Deal with fog value (r12 shadows oPos)...
;------------------------------------------------------------------------------
mov	oFog.x, -r12.w

