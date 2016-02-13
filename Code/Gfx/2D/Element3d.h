#ifndef __GFX_2D_ELEMENT3D_H__
#define __GFX_2D_ELEMENT3D_H__

#ifndef __GEL_OBJECT_H
#include <gel/object.h>
#endif
#include <gfx/2D/ScreenElement2.h>

namespace Gfx
{
    class CSkeleton;
};
                     
namespace Nx
{
    class CModel;
};

namespace Script
{
	class CScript;
	class CStruct;
};

namespace Front
{

#define MAX_MODELS_PER_ELEMENT 25

struct SSubModel
{
	Nx::CModel	*mpModel;
    Gfx::CSkeleton* mpSkeleton;
	Mth::Vector mOffset;
};
	
class CElement3d : public CScreenElement
{
	friend class CScreenElementManager;

	SSubModel mp_models[MAX_MODELS_PER_ELEMENT];
	int		m_num_models;
	
	float	m_camera_z;
	float	m_angle_x;
	float	m_angle_y;
	float	m_angle_z;
	float	m_angvel_x;
	float	m_angvel_y;
	float	m_angvel_z;

	Mth::Matrix m_model_matrix;
	
	enum EPointType
	{
		POINT_TYPE_NONE,
		POINT_TYPE_NODE,
		POINT_TYPE_OBJECT,
	};
	Mth::Vector m_node_position_to_point_to;
	EPointType m_point_type;
	
	enum EAngleOrder
	{
		ANGLE_ORDER_XYZ,
		ANGLE_ORDER_XZY,
		ANGLE_ORDER_YXZ,
		ANGLE_ORDER_YZX,
		ANGLE_ORDER_ZXY,
		ANGLE_ORDER_ZYX,
	};
	EAngleOrder m_angle_order;

	uint32 m_id_of_object_to_point_to;

	float m_hover_amp;
	int m_hover_period;

	Mth::Vector m_pivot_point;				// Point in local space for rotations

	// These following members are used when the element3d is associated with some 3d position.
	// The 3d poistion can be got from an instance of a CObject, or from a fixed node.

	// Offset from the parent position.
	Mth::Vector m_parent_offset;
		
	Mth::Vector m_offset_of_origin_from_center; // origin - center

	// different from 2D scale, is scale of 3D object
	float		m_scale3d;

	// These three are used for when the object needs to scale according to the distance from the camera.
	bool 	m_scale_with_distance;
	float 	m_scale_multiplier;
	float 	m_max_scale;

	int			m_active_viewport_number; // viewport to draw to
	
	uint32 m_parent_object_name;	// Non-zero if associated with an instance of a CObject
	int m_skater_number; 			// Valid if m_parent_object_name is skater
	uint32 m_parent_node_name;		// Non-zero if associated with a node.
	
	// Tilt value. Used for the arrow so that it does not go totally flat, which makes the
	// direction it is pointing in hard to see. Units are degrees.
	float m_tilt;
	
	// These is used to flag the element to be killed if its parent object dies.	
	bool m_waiting_to_die;
	
public:
				CElement3d();
	virtual		~CElement3d();

	void		SetProperties(Script::CStruct *p_props);
	void		SetMorph(Script::CStruct *pProps);

	void		UnloadModels();
	void		AddModel(const char *p_model_name, uint32 skeleton_name, Mth::Vector offset, int texDictOffset);
	void 		AddModelFromSector(uint32 sectorName, Mth::Vector offset);
    Nx::CModel* CreateModel( uint32 skeleton_name, Mth::Vector offset );
	void 		AutoComputeScale();
	
protected:

	void		update();
	void		update_visibility();
    Gfx::CSkeleton* CreateSkeleton( Nx::CModel* pModel, uint32 skeletonChecksum );

	static void	s_apply_rotations(Mth::Matrix & mat, float x_rot, float y_rot, float z_rot, EAngleOrder rot_order);
};


}

#endif

