#ifndef __BILLBOARD_H
#define __BILLBOARD_H


#include <core/defines.h>
#include "render.h"

namespace NxXbox
{

/******************************************************************/
/*                                                                */
/* A single billboard entry.									  */
/*                                                                */
/******************************************************************/
struct sBillboardEntry
{
	sBillboardData::EBillboardType	m_type;
	sMesh							*mp_mesh;
//	Mth::Vector						m_pivot_pos;
//	Mth::Vector						m_pivot_axis;	// Normalised axis of rotation, valid only for type ARBITRARY_AXIS_ALIGNED.
//	float							m_verts[4][3];	// Verts defined as an offset from the pivot point.

									sBillboardEntry( sMesh *p_mesh );
									~sBillboardEntry( void );
};
	

	
/******************************************************************/
/*                                                                */
/* Stores information about a billboard batch, effectively a set  */
/* of billboards with the same material.                          */
/*                                                                */
/******************************************************************/
struct sBillboardMaterialBatch
{
	int							m_entry_size;		// Push buffer size of each billboard entry (bytes).
	sMaterial					*mp_material;
	Lst::Head <sBillboardEntry>	*mp_entries[3];		// One entry each for screen aligned, y axis aligned, and arbitrary aligned.

								sBillboardMaterialBatch( sMaterial *p_material );
								~sBillboardMaterialBatch( void );

	bool						IsEmpty( void );
	float						GetDrawOrder( void );
	sMaterial					*GetMaterial( void );
	void						AddEntry( sMesh *p_mesh );
	void						RemoveEntry( sMesh *p_mesh );
	void						ProcessMesh( sMesh *p_mesh );
	void						Render( void );
	void						Reset( void );
};



/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
struct sBillboardManager
{
	enum
	{
				MAX_BILLBOARD_BATCHES	= 256
	};

	int							m_num_batches;
	sBillboardMaterialBatch		*mp_batches[MAX_BILLBOARD_BATCHES];

	Mth::Vector					m_at;
	Mth::Vector					m_screen_right;
	Mth::Vector					m_screen_up;
	Mth::Vector					m_at_xz;
	Mth::Vector					m_screen_right_xz;

								sBillboardManager( void );
								~sBillboardManager( void );

	void						Render( uint32 flags );
	void						SetCameraMatrix( void );
	sBillboardMaterialBatch		*GetBatch( sMaterial *p_material );
	void						AddEntry( sMesh *p_mesh );
	void						RemoveEntry( sMesh *p_mesh );
	void						SortBatches( void );
};


extern sBillboardManager BillboardManager;


	

} // namespace NxXbox


#endif // __BILLBOARD_H

