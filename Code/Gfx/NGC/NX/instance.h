#ifndef __INSTANCE_H
#define __INSTANCE_H


#include <core/defines.h>
#include <gfx/NxModel.h>
#include "scene.h"

namespace NxNgc
{

class CInstance
{
public:
	enum EInstanceFlag
	{
		INSTANCE_FLAG_DELETE_ATTACHED_SCENE	= 0x01,
		INSTANCE_FLAG_TRANSFORM_ME			= 0x02,
		INSTANCE_FLAG_LIGHTING_ALLOWED		= 0x04

	};
	
	void		SetTransform( Mth::Matrix &transform )	{ m_transform = transform; }
	Mth::Matrix	*GetTransform(void)						{ return &m_transform; }
	int			GetNumBones( void )						{ return m_num_bones; }
	Mth::Matrix	*GetBoneTransforms( void )				{ return mp_bone_transforms; }
	void		SetBoneTransforms( Mth::Matrix * p )	{ mp_bone_transforms = p; }
	sScene		*GetScene( void )						{ return mp_scene; }
	void		SetActive( bool active )				{ m_active = (uint16)active; }
	bool		GetActive( void )						{ return (bool)m_active; }
	void		SetFlag( EInstanceFlag flag )			{ m_flags |= flag; }
	uint32		GetFlags( void )						{ return m_flags; }
	void		ClearFlag( EInstanceFlag flag )			{ m_flags &= ~flag; }
	s16			*GetPosNormalBuffer( int buffer );
	void		SetVisibility( uint32 mask )			{ m_visibility_mask	= mask; }
	uint32		GetVisibility( void )					{ return m_visibility_mask; }
	CInstance	*GetNextInstance( void )				{ return mp_next_instance; }

	void		SetModel( Nx::CModel *p_model )			{ mp_model = p_model; }
	Nx::CModel	*GetModel( void )						{ return mp_model; }

	int			GetMeshesRendered( void )				{ return m_meshes_rendered; }
	void		SetMeshesRendered( int n )				{ m_meshes_rendered = n; }

	CInstance( sScene *pScene, Mth::Matrix &transform, int numBones, Mth::Matrix *pBoneTransforms );
	~CInstance();

	void		Transform( uint32 flags, ROMtx * p_mtx_buffer, Mth::Matrix *p_mtx_last );
	void		Render( uint32 flags );

	s16*		GetPosNormRenderBuffer() { return mp_posNormRenderBuffer; }
	void		SetPosNormRenderBuffer( s16* p ) { mp_posNormRenderBuffer = p; }

	#ifndef __NOPT_FINAL__
	int			m_object_num;
	#endif		// __NOPT_FINAL__

private:
	uint32		m_flags;
	Mth::Matrix	m_transform;
	Mth::Matrix	*mp_bone_transforms;
	int			m_num_bones;
	uint16		m_active;
	uint16		m_meshes_rendered;
	sScene		*mp_scene;

	uint32		m_visibility_mask;

	Nx::CModel	*mp_model;		// Required in order to get pointer to CXboxLights structure at render time.

	s16*		mp_posNormBuffer;
	s16*		mp_posNormRenderBuffer;

	CInstance	*mp_next_instance;
};


void	InitialiseInstanceTable( void );
void	render_instance( CInstance *p_instance, uint32 flags );
void	render_instances( uint32 flags );
void	process_instances( void );


} // namespace NxNgc


#endif // __INSTANCE_H

