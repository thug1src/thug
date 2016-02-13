xvs.1.1

; Constants:
;  c0 - c3		WVP Matrix (WORLD*VIEW*PROJECTION)
;  c4			Screen right vector
;  c5			Screen up vector
;  c8-c11		Vertex texture coordinates (for index0 through index3)
;  c12-c15		Vertex width and height multipliers (for index0 through index3)

;  c16			Particle position (xyz) (set in the data stream per particle)
;  c17			Particle start width and height (xy) and end width and height (zw) (set in the data stream per particle)
;  c18			Size interpolator (x) and color interpolator (y) (set in the data stream per particle)


; In:
;   v0 - Vertex start color
;   v1 - Vertex end color
;   v2 - Index

; Out:
;   oPos	- Position
;   oD0		- Vertex color
;   oT0		- TextureCoords

; Get the index of this vert
mov	a0.x, v2.x

; Move the width and height multipliers into a general register.
mov r1, c[12 + a0.x]

; Calculate the interpolated width and height of the particle.
mov r2, c17.zw		// r2 = ( c17.z, c17.w, c17.w, c17.w )
sub r2, r2, c17.xy	// r2 = ( c17.z - c17.x, c17.w - c17.y, ...)
mul r2, r2, c18.x
add r2, r2, c17.xy

; Multiply by the width and height of this particle.
mul r1, r1, r2

; Add width and height multiples of the screen right and up vectors
mul r0.xyz, r1.x, c4
mad r0.xyz, r1.y, c5, r0

; Add particle position
add r0.xyz, c16, r0
sge r0.w,	r1.x, r1.x		; r0.w = 1.0

; Vertex->screen transform
dp4 oPos.x, r0, c0
dp4 oPos.y, r0, c1
dp4 oPos.z, r0, c2
dp4 oPos.w, r0, c3

; Calculate the interpolated vertex color
mov r2, v1
sub r2, r2, v0
mad r2, r2, c18.y, v0

; Vertex color
mov oD0, r2

; Deal with fog value (r12 shadows oPos)...
mov	oFog.x, -r12.w

; Move texture coordinate in based on index
mov oT0, c[8 + a0.x]
