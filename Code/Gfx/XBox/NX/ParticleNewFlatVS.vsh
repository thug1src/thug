xvs.1.1

; Constants:
;  c0 - c3		WVP Matrix (WORLD*VIEW*PROJECTION)
;  c4			Screen right vector
;  c5			Screen up vector
;  c8-c11		Vertex texture coordinates (for index0 through index3)
;  c12-c15		Vertex width and height multipliers (for index0 through index3)

;  c16			Particle random seed vector
;  c17			Particle time interpolator (c17.x), color interpolator (c17.y)

;  c18-c20		Particle s0, s1 and s2 vectors
;  c21-c23		Particle p0, p1 and p2 vectors

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

; Move the time interpolator into a general register, and calculate the square.
mov r4, c17
mul r4.y, r4.x, r4.x

; Move the width and height multipliers into a general register.
mov r3, c[12 + a0.x]

; Calculate the position of the particle, from pos = ( m_p0 + ( t * m_p1 ) + (( t * t ) * m_p2 )) + ( m_s0 + ( t * m_s1 ) + (( t * t ) * m_s2 )).Scale( r );
mov r0, c21					; pos = m_p0
mad r0, c22, r4.x, r0		; pos = m_p0 + ( t * m_p1 )
mad r0, c23, r4.y, r0		; pos = m_p0 + ( t * m_p1 ) + (( t * t ) * m_p2 )

mov r1, c18					; tmp = ( m_s0 )
mad r1, c19, r4.x, r1		; tmp = ( m_s0 + ( t * m_s1 ))
mad r1, c20, r4.y, r1		; tmp = ( m_s0 + ( t * m_s1 ) + (( t * t ) * m_s2 ))

mad r0, r1, c16, r0			; pos = ( m_p0 + ( t * m_p1 ) + (( t * t ) * m_p2 )) + ( m_s0 + ( t * m_s1 ) + (( t * t ) * m_s2 )).Scale( r );

; Calculate the interpolated width and height of the particle.
mov r2, r0.w

; Now set position w to 1.0
sge r0.w, r0.x, r0.x

; Multiply width and height by the width and height multipliers of this vertex index
mul r2.xy, r2.xy, r3.xy

; Add width and height multiples of the screen right and up vectors to the position.
mad r0.xyz, r2.x, c4, r0
mad r0.xyz, r2.y, c5, r0

; Vertex->screen transform
dp4 oPos.x, r0, c0
dp4 oPos.y, r0, c1
dp4 oPos.z, r0, c2
dp4 oPos.w, r0, c3

; Calculate the interpolated vertex color
mov r2, v1
sub r2, r2, v0
mad oD0, r2, c17.y, v0

; Deal with fog value (r12 shadows oPos)...
mov	oFog.x, -r12.w

; Move texture coordinate in based on index
mov oT0, c[8 + a0.x]
