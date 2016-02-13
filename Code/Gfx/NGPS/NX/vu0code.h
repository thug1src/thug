#ifndef __VU0CODE_H
#define __VU0CODE_H

extern uint MPGStart0[8]; 		// Mick:  The [8] is a patch to make the compiler not assume it is in ldata
extern uint MPGEnd0[8];

extern uint TestFunc;
extern uint InitialiseOccluders;
extern uint TestSphereAgainstOccluders;
extern uint RayTriangleCollision;
extern uint BatchRayTriangleCollision;
extern uint ViewCullTest;
extern uint OuterCullTest;
extern uint BothCullTests;

#endif //__VU0CODE_H

