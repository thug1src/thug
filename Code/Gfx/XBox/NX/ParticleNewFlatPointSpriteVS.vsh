xvs.1.1

; Constants:
;  c0 - c3		WVP Matrix (WORLD*VIEW*PROJECTION)
;  c18-c20		Particle s0, s1 and s2 vectors
;  c21-c23		Particle p0, p1 and p2 vectors
;  c24			Eye position (x,y,z) and viewport height (w)

; In:
;   v0 - 4-element random vector
;   v1 - Time (x) and color interpolator (y)
;   v2 - Vertex start color
;   v3 - Vertex end color

; Out:
;   oPos	- Position
;	oPts	- Point sprite size
;   oD0		- Vertex color


; Calculate the square of the time interpolator.
mul r2.x, v1.x, v1.x

; Calculate the position of the particle, from pos = ( m_p0 + ( t * m_p1 ) + (( t * t ) * m_p2 )) + ( m_s0 + ( t * m_s1 ) + (( t * t ) * m_s2 )).Scale( r );
mov r0, c21					; pos = m_p0
mad r0, c22, v1.x, r0		; pos = m_p0 + ( t * m_p1 )
mad r0, c23, r2.x, r0		; pos = m_p0 + ( t * m_p1 ) + (( t * t ) * m_p2 )

mov r1, c18					; tmp = ( m_s0 )
mad r1, c19, v1.x, r1		; tmp = ( m_s0 + ( t * m_s1 ))
mad r1, c20, r2.x, r1		; tmp = ( m_s0 + ( t * m_s1 ) + (( t * t ) * m_s2 ))

mad r0, r1, v0, r0			; pos = ( m_p0 + ( t * m_p1 ) + (( t * t ) * m_p2 )) + ( m_s0 + ( t * m_s1 ) + (( t * t ) * m_s2 )).Scale( r );

; Calculate the distance from the camera to the particle.
sub r4, c24, r0
dp3	r4.x, r4, r4
rsq r4.x, r4.x
mul r4.x, r4.x, r0.w

; Now set position w to 1.0
sge r0.w, r0.x, r0.x

; Vertex->screen transform
dp4 oPos.x, r0, c0
dp4 oPos.y, r0, c1
dp4 oPos.z, r0, c2
dp4 oPos.w, r0, c3

; Save off the size of the particle
mul oPts.x, r4.x, c24.w

; Calculate the interpolated vertex color
mov r2, v2					; r2 = start_col
sub r3, v3, r2				; r3 = end_col - start_col
mad oD0, r3, v1.y, r2		; r3 = start_col + (( end col - start_col ) * color_interpolator )

; Deal with fog value (r12 shadows oPos)...
mov	oFog.x, -r12.w
