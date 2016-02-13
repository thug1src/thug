#ifndef __RENDER_H
#define __RENDER_H


#include "mikemath.h"
#include <core/math.h>

namespace NxPs2
{


#define SUB_INCH_PRECISION 16.0f
#define RECIPROCAL_SUB_INCH_PRECISION 0.0625f


typedef struct
{
	float tx,ty;
	float n,f;
}
sFrustum;


void RenderPrologue(void);
void RenderInitImmediateMode();

void RenderWorld(Mth::Matrix* camera_orient, Mth::Vector* camera_pos, float view_angle, float screen_aspect);
void RenderEpilogue(void);

void RenderSwitchImmediateMode();

void RenderViewport(uint viewport_num, const Mth::Matrix& camera_transform, const Mth::Rect & viewport_rec, float view_angle,
					float viewport_aspect, float nearz, float farz, bool just_setup=false);

bool IsVisible(Mth::Vector &center, float radius);
void UpdateCamera(Mat *camera_orient, Vec *camera_pos);
void SendDisplayList(void);
void RenderMesh(struct sMesh *pMesh, uint RenderFlags);
void SkinTest(void);
void SetTextureProjectionCamera( Mth::Vector *p_pos, Mth::Vector *p_at );
void DrawRectangle(int x0, int y0, int x1, int y1, uint32 rgba);

void Clear2DVRAM();
void VRAMNeedsUpdating();
void Reallocate2DVRAM();
void Update2DDMA();

void EnableFlipCopy(bool flip);
bool FlipCopyEnabled();


extern uint  Frame, Field;
extern Mat SkaterTransform;
extern Mat BoneTransforms[64];
extern int NumBones;

extern int	SingleRender;
extern bool DoLetterbox;
extern int	DMAOverflowOK;




namespace render
{
	void SetupVars(uint viewport_num, const Mth::Matrix& camera_transform, const Mth::Rect& viewport_rec, float view_angle,
				   float viewport_aspect, float near_z, float far_z);
#ifndef __PLAT_WN32__
	void SetupVarsDMA(uint viewport_num, float near, float far);	
#endif		// __PLAT_WN32__
	bool VarsUpToDate(uint viewport_num);
	void InvalidateVars();

	void OldStyle();
	void NewStyle(bool just_setup = false);

	extern Mth::Matrix WorldToCamera;
	extern Mth::Matrix WorldToCameraRotation;
	extern Mth::Matrix WorldToFrustum;
	extern Mth::Matrix WorldToViewport;
	extern Mth::Matrix WorldToIntViewport;
	extern Mth::Matrix CameraToWorld;
	extern Mth::Matrix CameraToWorldRotation;
	extern Mth::Matrix CameraToFrustum;
	extern Mth::Matrix CameraToViewport;
	extern Mth::Matrix FrustumToViewport;
	extern Mth::Matrix FrustumToIntViewport;
	extern Mth::Matrix AdjustedWorldToCamera;
	extern Mth::Matrix AdjustedWorldToFrustum;
	extern Mth::Matrix AdjustedWorldToViewport;
	extern Mth::Matrix AdjustedWorldToIntViewport;
	extern Mth::Matrix SkyWorldToCamera;
	extern Mth::Matrix SkyWorldToFrustum;
	extern Mth::Matrix SkyWorldToViewport;
	extern Mth::Matrix SkyFrustumToViewport;

	extern Mth::Vector ViewportScale;
	extern Mth::Vector ViewportOffset;
	extern Mth::Vector IntViewportScale;
	extern Mth::Vector IntViewportOffset;
	extern Mth::Vector InverseViewportScale;
	extern Mth::Vector InverseViewportOffset;
	extern Mth::Vector InverseIntViewportScale;
	extern Mth::Vector InverseIntViewportOffset;
	extern Mth::Vector SkyViewportScale;
	extern Mth::Vector SkyViewportOffset;
	extern Mth::Vector SkyInverseViewportScale;
	extern Mth::Vector SkyInverseViewportOffset;
	extern Mth::Vector RowReg;
	extern Mth::Vector RowRegI;
	extern Mth::Vector RowRegF;
	extern Mth::Vector ShadowCameraPosition;
	extern Mth::Vector AltFrustum;
	extern Mth::Vector CameraPosition;

	extern uint64 reg_XYOFFSET;
	extern uint64 reg_SCISSOR;

	extern float sx,sy,cx,cy,tx,ty,Sx,Sy,Cx,Cy,Tx,Ty,Near,Far;
	extern float sMultipassMaxDist;

	extern uint Frame, Field;
	extern uint ViewportMask;

	extern bool FrustumFlagsPlus[3], FrustumFlagsMinus[3];

	#define MAX_SORTED_LIST_MARKERS 32
	extern int sSortedListMarker[MAX_SORTED_LIST_MARKERS];
	extern int sMarkerIndex;
	extern int sTime;
	extern int sTotalNewParticles;

	extern float FogNear, FogAlpha;
	extern uint32 FogCol;
	extern uint32 *p_patch_FOGCOL;
	extern bool EnableFog;
	extern float EffectiveFogAlpha;
	extern uint8 ShadowDensity;
}


} // namespace NxPs2

#endif // __RENDER_H
