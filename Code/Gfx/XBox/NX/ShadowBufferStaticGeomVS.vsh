xvs.1.1

; Constants:
;  c0  - c3  - WVP Matrix (WORLD*VIEW*PROJECTION)
;  c4  - c7  - WT Matrix (WORLD*TEXTURETRANSFORM)
;  c8  - local space light position.

; In:
;   v0 - Position
;   v1 - Vertex color
;   v2 - TexCoord0

; Out:
;   oPos - Position
;   oTn - TextureCoords

;vertex->screen
dp4 oPos.x, v0, c0
dp4 oPos.y, v0, c1
dp4 oPos.z, v0, c2
dp4 oPos.w, v0, c3

;diffuse lighting (not necessary for our purposes)
;add	r0,c8,-v0
;dp3 r0.w,r0,r0
;rsq r1.x,r0.w
;mul r0,r0,r1.x
;dp3 oD0,v1,r0

;decal texture
mov oT0, v2

;vertex->shadowbuffer texcoords
dp4 oT1.x, v0, c4
dp4 oT1.y, v0, c5
dp4 oT1.z, v0, c6
dp4 r0.w, v0, c7

;clamp w (q) to 0
slt r1, c0, c0
max r0.w, r0.w, r1.w
mov oT1.w, r0.w
