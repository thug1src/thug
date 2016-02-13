#ifndef __ANIM_H
#define __ANIM_H

extern DWORD WeightedMeshVS_VXC_1Weight;
extern DWORD WeightedMeshVS_VXC_2Weight;
extern DWORD WeightedMeshVS_VXC_3Weight;
extern DWORD WeightedMeshVS_VXC_Specular_1Weight;
extern DWORD WeightedMeshVS_VXC_Specular_2Weight;
extern DWORD WeightedMeshVS_VXC_Specular_3Weight;
extern DWORD WeightedMeshVS_VXC_1Weight_UVTransform;
extern DWORD WeightedMeshVS_VXC_2Weight_UVTransform;
extern DWORD WeightedMeshVS_VXC_3Weight_UVTransform;
extern DWORD WeightedMeshVS_VXC_1Weight_SBPassThru;
extern DWORD WeightedMeshVS_VXC_2Weight_SBPassThru;
extern DWORD WeightedMeshVS_VXC_3Weight_SBPassThru;
extern DWORD WeightedMeshVertexShader_SBWrite;
extern DWORD BillboardScreenAlignedVS;
extern DWORD ParticleFlatVS;
extern DWORD ParticleNewFlatVS;
extern DWORD ParticleNewFlatPointSpriteVS;
extern DWORD ShadowBufferStaticGeomVS;

namespace NxXbox
{

DWORD	GetVertexShader( bool vertex_colors, bool specular, uint32 max_weights_used );
void	CreateWeightedMeshVertexShaders( void );
void	startup_weighted_mesh_vertex_shader( void );
void	setup_weighted_mesh_vertex_shader( void *p_root_matrix, void *p_bone_matrices, int num_bone_matrices );
void	shutdown_weighted_mesh_vertex_shader( void );


} // namespace NxXbox

#endif // __ANIM_H

