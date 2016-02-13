#ifndef __LINE_H
#define __LINE_H


namespace NxPs2
{


void BeginLines2D(uint32 rgba);
void DrawLine2D(float x0, float y0, float z0, float x1, float y1, float z1);
void EndLines2D(void);
void BeginLines3D(uint32 rgba);
void ChangeLineColor(uint32 rgba);
void DrawLine3D(float x0, float y0, float z0, float x1, float y1, float z1);
void EndLines3D(void);

} // namespace NxPs2


#endif // __LINE_H

