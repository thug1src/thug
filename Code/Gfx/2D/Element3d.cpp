#include <core/defines.h>
#include <gfx/2D/Element3d.h>
#include <gfx/2D/ScreenElemMan.h>
#include <gel/assman/assman.h>
#include <gel/components/modelcomponent.h>
#include <gel/object/compositeobject.h>
#include <gel/object/compositeobjectmanager.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/script.h>
#include <gel/object/compositeobjectmanager.h>
#include <sk/scripting/nodearray.h>		// <<<<<<<< NEEDS MOVING!!!!
#include <gfx/modelappearance.h>
#include <gfx/modelbuilder.h>
#include <gfx/skeleton.h>
#include <gfx/nx.h>
#include <gfx/nxgeom.h>
#include <gfx/nxmodel.h>
#include <gfx/nxviewman.h>
#include <gfx/gfxutils.h>
#include <sys/config/config.h>
#include <sys/replay/replay.h>

namespace Front
{

// added by your mother
// returns the scale that needs to be applied to (modelWorldW,modelWorldH) to make it fit (screenW,screenH)
static float sGetScaleFromScreenDimensions(float screenW, float screenH, float zOffset, float modelWorldW, float modelWorldH)
{
	// Inverse project.
	float desired_world_w = screenW * zOffset / (640.0f * (1.0f + screenW / 1280.0f));
	float desired_world_h = screenH * zOffset / (448.0f * (1.0f + screenW / 896.0f));

	float scale_x = desired_world_w / modelWorldW;
	float scale_y = desired_world_h / modelWorldH;
	return (-scale_x < -scale_y) ? -scale_x : -scale_y;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// screenX, screenY indicate where the origin of the model will be located
static void sGetWorldMatrixFromScreenPosition(int camera_num, Mth::Matrix *p_world_matrix, float screenX, float screenY, float zOffset, Mth::Vector &model_offs)
{
	Dbg_MsgAssert(p_world_matrix,("NULL p_world_matrix"));
	Dbg_MsgAssert((camera_num >= 0) && (camera_num < Nx::CViewportManager::vMAX_NUM_ACTIVE_VIEWPORTS), ("Camera number is out of range: %d", camera_num));

	Mth::Vector camera_space_pos;

#ifdef __PLAT_XBOX__
	// Don't think Mick's assumption below is correct - for 3D objects going through the rendering pipeline, no screen correction is required.
	screenX = screenX - ( 640 / 2 );
	screenY = screenY - ( 448 / 2 );

	// Adjust X for aspect ratio that is different than 4/3 (1.3333333)
	screenX *= (Nx::CViewportManager::sGetScreenAspect() / 1.3333333f);

	// Also, adjust for 720p (horrible hack).
	if( NxXbox::EngineGlobals.backbuffer_width > 640 )
	{
		screenX *= 0.9f;
		screenY *= 0.9f;
	}

	camera_space_pos.Set( -screenX * zOffset / 448.0f, screenY * zOffset / 448.0f, zOffset );
#else
	// Mick:  Need to convert from logical to physical screen coordinates
	screenX = SCREEN_CONV_X(screenX);
	screenY = SCREEN_CONV_Y(screenY);

	// Make ScreenX and ScreenY relative to the centre of the screen.
	screenX -= SCREEN_CONV_X(640)/2;
	screenY -= SCREEN_CONV_Y(448)/2;

	// Adjust X for aspect ratio that is different than 4/3 (1.3333333)
	screenX *= (Nx::CViewportManager::sGetScreenAspect() / 1.3333333f);

	// Inverse project.
	camera_space_pos.Set(-screenX * zOffset / SCREEN_CONV_X(448),
						 screenY * zOffset / SCREEN_CONV_Y(448),
						 zOffset);
#endif // __PLAT_XBOX__

	camera_space_pos += model_offs;
						 
	// We now have a 3d vector in camera space, so rotate by the camera matrix
	// to get world coords.	

	p_world_matrix->Ident();

	// Get the camera.
	Gfx::Camera *p_camera = Nx::CViewportManager::sGetActiveCamera(camera_num);
	if (p_camera)
	{
		Mth::Matrix cam_matrix=p_camera->GetMatrix();
		
		// Camera matrix might have been incorrectly set up with a translation in W
		// so clear it out to be safe
		// Probably should be handled at a higher level.
		cam_matrix[W].Set();

		// Apply the camera matrix to the camera_space_pos
		Mth::Vector world_pos = camera_space_pos * cam_matrix;
		// and add the camera position to get the final world position.
		Mth::Vector cam_pos = p_camera->GetPos();
		cam_pos[W] = 0.0f;
		world_pos += cam_pos;
		world_pos[W] = 0.0f;
		
	
		// Make the rotation part of the passed matrix be the camera matrix so that the model 
		// always faces the camera.
		*p_world_matrix*=cam_matrix;
		// Then add in the calculated world position.
		p_world_matrix->Translate(world_pos);
	}	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CElement3d::CElement3d()
{
	for (int i=0; i<MAX_MODELS_PER_ELEMENT; ++i)
	{
		mp_models[i].mpModel=NULL;
        mp_models[i].mpSkeleton=NULL;
		mp_models[i].mOffset.Set();
	}
	m_num_models=0;
	
	m_camera_z=-400;
	m_angle_x=0.0f;
	m_angle_y=0.0f;
	m_angle_z=0.0f;
	m_angvel_x=0.0f;
	m_angvel_y=0.0f;
	m_angvel_z=0.0f;
	m_point_type=POINT_TYPE_NONE;
	m_angle_order=ANGLE_ORDER_YXZ;
	m_node_position_to_point_to.Set();
	m_id_of_object_to_point_to=0;
	m_tilt=0.0f;
	
	m_parent_object_name=0;
	m_skater_number=0;

	m_scale3d = 1.0f;
	m_active_viewport_number = 0;

	m_scale_with_distance=false;
	m_scale_multiplier=0.002f;
	m_max_scale=3.0f;

	m_waiting_to_die=false;	
	m_model_matrix.Ident();
	
	m_pivot_point.Set();
	m_offset_of_origin_from_center.Set();

	SetType(CScreenElement::TYPE_ELEMENT_3D);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CElement3d::~CElement3d()
{
	UnloadModels();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CElement3d::UnloadModels()
{
	for (int i=0; i<MAX_MODELS_PER_ELEMENT; ++i)
	{
		if (mp_models[i].mpModel)
		{
			Nx::CEngine::sUninitModel(mp_models[i].mpModel);
			mp_models[i].mpModel=NULL;
		}	

        if (mp_models[i].mpSkeleton)
        {
            delete mp_models[i].mpSkeleton;
        }

		mp_models[i].mOffset.Set();
	}
	m_num_models=0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Gfx::CSkeleton* CElement3d::CreateSkeleton( Nx::CModel* pModel, uint32 skeletonChecksum )
{
    // temporarily moved this function from CModel,
    // in order to make it easier to split off
    // skeleton into its own component.

	Gfx::CSkeletonData* pSkeletonData = (Gfx::CSkeletonData*) Ass::CAssMan::Instance()->GetAsset( skeletonChecksum, false );

	if ( !pSkeletonData )
	{
		Dbg_MsgAssert( 0, ("Unrecognized skeleton %s. (Is skeleton.q up to date?)", Script::FindChecksumName(skeletonChecksum)) );
	}
    
	Gfx::CSkeleton* pSkeleton = new Gfx::CSkeleton( pSkeletonData );
    
    Dbg_Assert( pSkeleton );
    Dbg_Assert( pSkeleton->GetNumBones() > 0 );
	
    pModel->SetSkeleton( pSkeleton );

    return pSkeleton;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Nx::CModel* CElement3d::CreateModel( uint32 skeleton_name, Mth::Vector offset )
{
	Dbg_MsgAssert(m_num_models<MAX_MODELS_PER_ELEMENT,("Reached max models for one CElement3d"));

	Nx::CModel *p_new_model=Nx::CEngine::sInitModel();
	Dbg_MsgAssert(p_new_model,("Nx::CEngine::sInitModel() returned NULL"));

	if ( skeleton_name )
	{
		// load a skeleton, if one was specified
		mp_models[m_num_models].mpSkeleton=this->CreateSkeleton( p_new_model, skeleton_name );
	}

	p_new_model->SetColor(m_rgba.r,m_rgba.g,m_rgba.b,m_rgba.a);

	Dbg_MsgAssert(mp_models[m_num_models].mpModel==NULL,("mp_models[m_num_models].mpModel not NULL ?"));
	mp_models[m_num_models].mpModel=p_new_model;
	
	mp_models[m_num_models].mOffset=offset;
	++m_num_models;

	return p_new_model;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CElement3d::AddModel(const char *p_model_name, uint32 skeleton_name, Mth::Vector offset, int texDictOffset)
{
	Dbg_MsgAssert(m_num_models<MAX_MODELS_PER_ELEMENT,("Reached max models for one CElement3d"));
		
	Nx::CModel *p_new_model=Nx::CEngine::sInitModel();
	Dbg_MsgAssert(p_new_model,("Nx::CEngine::sInitModel() returned NULL"));
	
	if ( skeleton_name )
	{
		// load a skeleton, if one was specified
		mp_models[m_num_models].mpSkeleton=this->CreateSkeleton( p_new_model, skeleton_name );
    }

	if ( p_model_name )
	{
		// loads a geom, if one was specified
		Str::String fullModelName;
		fullModelName = Gfx::GetModelFileName(p_model_name, ".mdl");
		p_new_model->AddGeom(fullModelName.getString(), 0, true, texDictOffset);
	}
	
	p_new_model->SetColor(m_rgba.r,m_rgba.g,m_rgba.b,m_rgba.a);
	
	Dbg_MsgAssert(mp_models[m_num_models].mpModel==NULL,("mp_models[m_num_models].mpModel not NULL ?"));
	mp_models[m_num_models].mpModel=p_new_model;
	
	mp_models[m_num_models].mOffset=offset;
	++m_num_models;

	update_visibility();
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CElement3d::AddModelFromSector(uint32 sectorName, Mth::Vector offset)
{
	Dbg_MsgAssert(m_num_models<MAX_MODELS_PER_ELEMENT,("Reached max models for one CElement3d"));
		
	Nx::CModel *p_new_model=Nx::CEngine::sInitModel();
	Dbg_MsgAssert(p_new_model,("Nx::CEngine::sInitModel() returned NULL"));
	
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(sectorName);
	Dbg_MsgAssert( p_sector, ( "This is not my beautiful CSector. sGetSector(0x%x) returned NULL (%s)\n", sectorName, Script::FindChecksumName(sectorName) ) );

	if ( p_sector )
	{
		// need to clone the source, not the instance?
		Nx::CGeom* p_geom = p_sector->GetGeom();
		Dbg_Assert(p_geom);
		if( p_geom )
		{
			Nx::CGeom* p_cloned_geom = p_geom->Clone( true );
			p_cloned_geom->SetActive(true);
			p_new_model->AddGeom( p_cloned_geom, 0 );
		}
	}
	
	p_new_model->SetColor(m_rgba.r,m_rgba.g,m_rgba.b,m_rgba.a);
	
	Dbg_MsgAssert(mp_models[m_num_models].mpModel==NULL,("mp_models[m_num_models].mpModel not NULL ?"));
	mp_models[m_num_models].mpModel=p_new_model;
	
	mp_models[m_num_models].mOffset=offset;
	++m_num_models;

	update_visibility();
}


void CElement3d::AutoComputeScale()
{	
	Mth::Vector element_world_min, element_world_max;
	for (int i = 0; i < m_num_models; i++)
	{
		Nx::CGeom *p_geom = mp_models[i].mpModel->GetGeom(0);
		Dbg_Assert(p_geom);
		Mth::CBBox b_box = p_geom->GetBoundingBox();
		Mth::Vector model_min(mp_models[i].mOffset.GetX() + b_box.GetMin().GetX(),
							  mp_models[i].mOffset.GetY() + b_box.GetMin().GetY(),
							  mp_models[i].mOffset.GetZ() + b_box.GetMin().GetZ());
		Mth::Vector model_max(mp_models[i].mOffset.GetX() + b_box.GetMax().GetX(),
							  mp_models[i].mOffset.GetY() + b_box.GetMax().GetY(),
							  mp_models[i].mOffset.GetZ() + b_box.GetMax().GetZ());

		if (i == 0)
		{
			element_world_min = model_min;
			element_world_max = model_max;
		}
		else
		{
			if (model_min[X] < element_world_min[X])
				element_world_min[X] = model_min[X];
			if (model_min[Y] < element_world_min[Y])
				element_world_min[Y] = model_min[Y];
			if (model_min[Z] < element_world_min[Z])
				element_world_min[Z] = model_min[Z];

			if (model_max[X] > element_world_max[X])
				element_world_max[X] = model_max[X];
			if (model_max[Y] > element_world_max[Y])
				element_world_max[Y] = model_max[Y];
			if (model_max[Z] > element_world_max[Z])
				element_world_max[Z] = model_max[Z];
		}

		/*
		Ryan("offset is (%.2f,%.2f,%.2f), min is (%.2f,%.2f,%.2f), max is (%.2f,%.2f,%.2f)\n",
			 mp_models[i].mOffset.GetX(), mp_models[i].mOffset.GetY(), mp_models[i].mOffset.GetZ(),
			 b_box.GetMin().GetX(), b_box.GetMin().GetY(), b_box.GetMin().GetZ(),
			 b_box.GetMax().GetX(), b_box.GetMax().GetY(), b_box.GetMax().GetZ());
		*/
	}

	float element_world_w = element_world_max[X] - element_world_min[X];
	float element_world_h = element_world_max[Y] - element_world_min[Y];
	float element_world_l = element_world_max[Z] - element_world_min[Z];
	m_scale3d = sGetScaleFromScreenDimensions(m_base_w, m_base_h, 
								  m_camera_z, 
								  (element_world_w > element_world_l) ? element_world_w : element_world_l,
									element_world_h);
	
	/*
	Ryan("yo homies, world dims are: (%.2f,%.2f,%.2f), desired scale is: (%.5f,%.5f), camera_z1 is %.4f, camera_z2 is %.4f\n", 
		 element_world_w, element_world_h, element_world_l,
		 scale, scale, m_camera_z, m_camera_z + element_world_max[Z] * scale);
	*/
	//CScreenElement::SetScale(scale, scale, false, CScreenElement::FORCE_INSTANT);

	m_offset_of_origin_from_center.Set(-(element_world_max[X] + element_world_min[X]) / 2.0f,
									   -(element_world_max[Y] + element_world_min[Y]) / 2.0f,
									   -(element_world_max[Z] + element_world_min[Z]) / 2.0f,
									   0.0f);
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// This gets called by the SetProps script command.
// Also gets called when the element is first created by CreateScreenElement, all the params sent
// to CreateScreenElement get dent to this function.
void CElement3d::SetProperties(Script::CStruct *p_props)
{
	CScreenElement::SetProperties(p_props);
	
	uint32 skeletonChecksum = 0;
	p_props->GetChecksum( CRCD(0x9794932,"skeletonName"), &skeletonChecksum, false );
	
	const char* p_model_name="";
	Script::CArray *p_model_array=NULL;
	uint32 clone_model_name;
	uint32 sector_name;
	Script::CArray *p_sector_array=NULL;
	bool skip_empty_model = false;
    
    int texDictOffset = 0;
    p_props->GetInteger( CRCD(0xf891ac27,"TexDictOffset"), &texDictOffset, false );
	
	if (p_props->GetChecksum( CRCD(0x73ec4c8,"CloneModel"), &clone_model_name ))
	{
		UnloadModels();
		
		Mth::Vector off;
		off.Set();

		// Clone the geometry off of an existing model
		Obj::CCompositeObject* pObject = static_cast<Obj::CCompositeObject *>(Obj::CCompositeObjectManager::Instance()->GetObjectByID(clone_model_name));
		Dbg_MsgAssert( pObject, ( "Couldn't find object id %d to clone", clone_model_name ) );

		Obj::CModelComponent *p_src_model_comp = GetModelComponentFromObject(pObject);
		Dbg_MsgAssert( p_src_model_comp, ( "Couldn't find model component in object id %d", clone_model_name ) );
		Dbg_MsgAssert( p_src_model_comp->GetModel(), ( "Couldn't find CModel in model component of object id %d", clone_model_name ) );

		Script::CArray *p_geom_array=NULL;
		if (p_props->GetArray(CRCD(0x3cc0910b,"CloneModelGeoms"), &p_geom_array))
		{
			// Create new model
			Nx::CModel *p_model = CreateModel(skeletonChecksum, off);

			uint32 geomName;
			for (uint i = 0; i < p_geom_array->GetSize(); i++)
			{
				geomName = p_geom_array->GetChecksum(i);
				if (geomName)
				{
					Nx::CGeom *p_orig_geom = p_src_model_comp->GetModel()->GetGeom(geomName);
					Dbg_MsgAssert(p_orig_geom, ("Couldn't find CGeom %s", Script::FindChecksumName(geomName)));

					Nx::CGeom *p_new_geom = p_orig_geom->Clone(true, p_model);
					Dbg_MsgAssert(p_new_geom, ("Couldn't clone CGeom %s", Script::FindChecksumName(geomName)));

					p_new_geom->SetActive(true);
					p_model->AddGeom(p_new_geom, geomName);
				}
			}
		}
		else
		{
			Dbg_MsgAssert( 0, ( "Must name geoms for object id %d to clone", clone_model_name ) );
		}

		update_visibility();
		skip_empty_model = true;
	}	
	else if (p_props->GetString( CRCD(0x286a8d26,"model"), &p_model_name ))
	{
		UnloadModels();
		
		Mth::Vector off;
		off.Set();
		AddModel(p_model_name,skeletonChecksum,off,texDictOffset);
	}	
	else if (p_props->GetArray(CRCD(0x1b29cff6,"models"),&p_model_array))
	{
		UnloadModels();
		for (uint32 i=0; i<p_model_array->GetSize(); ++i)
		{
			Script::CStruct *p_struct=p_model_array->GetStructure(i);
			p_struct->GetString(NONAME,&p_model_name);
			Mth::Vector off;
			off.Set();
			p_struct->GetVector(NONAME,&off);
			AddModel(p_model_name,skeletonChecksum,off,texDictOffset);
		}
	}	 
	if (p_props->GetChecksum( CRCD(0xb45c2617,"sector"), &sector_name ))
	{
		UnloadModels();
		
		Mth::Vector off;
		off.Set();
		AddModelFromSector(sector_name, off);
	}	
	else if (p_props->GetArray(CRCD(0x4a6bf967,"sectors"), &p_sector_array))
	{
		UnloadModels();
		for (uint32 i=0; i < p_sector_array->GetSize(); ++i)
		{
			Script::CStruct *p_struct = p_sector_array->GetStructure(i);
			p_struct->GetChecksum(NONAME, &sector_name);
			Mth::Vector off;
			off.Set();
			p_struct->GetVector(NONAME, &off);
			AddModelFromSector(sector_name, off);
		}
	}	 
	else if ( (skeletonChecksum != 0) && !skip_empty_model )
	{
		// no model was specified, but we might need to create
		// a skeleton anyway (like for the preview models)
		Mth::Vector off;
		off.Set();
		AddModel(NULL, skeletonChecksum, off, 0);
	}
	
	p_props->GetFloat(CRCD(0xed7c6031,"CameraZ"),&m_camera_z);
	p_props->GetFloat(CRCD(0xaffd09d,"AngleX"),&m_angle_x);
	p_props->GetFloat(CRCD(0x7df8e00b,"AngleY"),&m_angle_y);
	p_props->GetFloat(CRCD(0xe4f1b1b1,"AngleZ"),&m_angle_z);
	p_props->GetFloat(CRCD(0xc1608989,"AngVelX"),&m_angvel_x);
	p_props->GetFloat(CRCD(0xb667b91f,"AngVelY"),&m_angvel_y);
	p_props->GetFloat(CRCD(0x2f6ee8a5,"AngVelZ"),&m_angvel_z);

	if (p_props->ContainsFlag(CRCD(0x233f6b18,"AngleOrderXYZ"))) m_angle_order = ANGLE_ORDER_XYZ;
	if (p_props->ContainsFlag(CRCD(0x911b6961,"AngleOrderXZY"))) m_angle_order = ANGLE_ORDER_XZY;
	if (p_props->ContainsFlag(CRCD(0x3be6306e,"AngleOrderYXZ"))) m_angle_order = ANGLE_ORDER_YXZ;
	if (p_props->ContainsFlag(CRCD(0xe7de33c0,"AngleOrderYZX"))) m_angle_order = ANGLE_ORDER_YZX;
	if (p_props->ContainsFlag(CRCD(0xa0a9df8d,"AngleOrderZXY"))) m_angle_order = ANGLE_ORDER_ZXY;
	if (p_props->ContainsFlag(CRCD(0xceb5de5a,"AngleOrderZYX"))) m_angle_order = ANGLE_ORDER_ZYX;

	if (p_props->ContainsFlag(CRCD(0xf01fdca4,"DisablePointing")))
	{
		m_point_type=POINT_TYPE_NONE;
	}
		
	uint32 node_checksum=0;
	if (p_props->GetChecksum(CRCD(0xb2a86eb5,"NodeToPointTo"),&node_checksum))
	{
		int node_to_point_to=SkateScript::FindNamedNode(node_checksum);
		Script::CStruct *p_node=SkateScript::GetNode(node_to_point_to);
		Dbg_MsgAssert(p_node,("NULL p_node"));
		SkateScript::GetPosition(p_node,&m_node_position_to_point_to);

		m_point_type=POINT_TYPE_NODE;
	}	

	m_id_of_object_to_point_to=0;
	if (p_props->GetChecksum(CRCD(0xdad39b88,"ObjectToPointTo"),&m_id_of_object_to_point_to))
	{
		m_point_type=POINT_TYPE_OBJECT;
	}	
	
	m_tilt=0.0f;
	p_props->GetFloat(CRCD(0xe3c07609,"Tilt"),&m_tilt);
	
	m_hover_amp=0.0f;
	m_hover_period=0;
	Script::CStruct *p_hover_params=NULL;
	if (p_props->GetStructure(CRCD(0xca1c41,"HoverParams"),&p_hover_params))
	{
		p_hover_params->GetFloat(CRCD(0xc9fde32c,"Amp"),&m_hover_amp);
		float f=1.0f;
		p_hover_params->GetFloat(CRCD(0xa80bea4a,"Freq"),&f);
		m_hover_period=(int)(1000.0f/f);
	}
	
	m_skater_number=0;
	m_parent_object_name=0;
	m_parent_node_name=0;
	m_parent_offset.Set();
	Script::CStruct *p_parent_params=NULL;
	if (p_props->GetStructure(CRCD(0x36388933,"ParentParams"),&p_parent_params))
	{
		p_parent_params->GetChecksum(CRCD(0xa1dc81f9,"Name"),&m_parent_object_name);
		p_parent_params->GetVector(NONAME,&m_parent_offset);
		if (p_parent_params->GetInteger(CRCD(0x5b8ab877,"Skater"),&m_skater_number))
		{
			m_parent_object_name=0x5b8ab877/*Skater*/;
		}	
		// Sometimes they want an arrow to be hovering near an actual node,
		// rather than the instance of some CObject associated with that node.
		// Eg, they may want an arrow hovering over some level geometry.
		p_parent_params->GetChecksum(CRCD(0x7a8017ba,"Node"),&m_parent_node_name);
		
		Dbg_MsgAssert(!(m_parent_node_name && m_parent_object_name),("Cannot specify both a parent object and a parent node for an element3d"));
	}

	if (p_props->ContainsFlag(CRCD(0x1b399065,"scale_to_screen_dims")))
	{
		AutoComputeScale();
	}
	else if (p_props->GetVector("PivotPoint", &m_pivot_point))
	{
		m_pivot_point[W] = 1.0f;		// force to be a point
	}

	m_scale_with_distance=p_props->ContainsFlag(CRCD(0x4564c8f2,"ScaleWithDistance"));
	m_scale_multiplier=0.002f;
	p_props->GetFloat(CRCD(0x571f3f3a,"ScaleMultiplier"),&m_scale_multiplier);
	m_max_scale=3.0f;
	p_props->GetFloat(CRCD(0x31e78f98,"MaxScale"),&m_max_scale);


	int viewport;
	if (p_props->GetInteger(CRCD(0x655ee08e,"Active_Viewport"), &viewport))
	{
		m_active_viewport_number = viewport;
		update_visibility();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CElement3d::update()
{
	if (m_waiting_to_die)
	{
		// Return straight away to prevent repeated spawnings of the KillElement3d script
		return;
	}
		
	if (m_num_models)
	{							
		if (m_parent_object_name)
		{
			// Get a pointer to the parent object.
			Obj::CMovingObject *p_pos_obj=NULL;

// Skate module 
//			if (m_parent_object_name==0x5b8ab877/*Skater*/)
//			{
//				// This is 
//				Mdl::Skate * skate_mod = Mdl::Skate::Instance();
//				p_pos_obj=skate_mod->GetSkater(m_skater_number);
//			}
//			else			
			{
//				p_pos_obj=Obj::CMovingObject::m_hash_table.GetItem(m_parent_object_name);
				p_pos_obj = (Obj::CMovingObject*) Obj::CCompositeObjectManager::Instance()->GetObjectByID(m_parent_object_name);
			}	
			
			if (p_pos_obj)
			{
				m_model_matrix.Ident();
				m_model_matrix.SetPos(p_pos_obj->m_pos+m_parent_offset*m_model_matrix);
			}
			else
			{
				// The parent object has disappeared!
				// So call the KillElement3d script to safely delete this arrow.
				
				// Tried various ways of killing directly from c-code, but they didn't work.
				// Instant deletion here will mess up the update loop, and calling
				// Die() to flag the element for deletion later also cause problems, because
				// the parent element needs to be unlocked at the time of deletion.
				
				// Running a script seems to be safe ...
				// The KillElement3d script will wait a gameframe, then call the usual Die
				Script::CScript *p_script=Script::SpawnScript(0xa66c1c31/*KillElement3d*/);
				#ifdef __NOPT_ASSERT__
				p_script->SetCommentString("Created by CElement3d::update()");
				#endif
				p_script->mpObject=this;
				m_waiting_to_die=true;
			}	
		}
		else if (m_parent_node_name)
		{
			// FindNamedNode will assert if it does not find the node.
			int node_index=SkateScript::FindNamedNode(m_parent_node_name);
			Script::CStruct *p_node=SkateScript::GetNode(node_index);
			Dbg_MsgAssert(p_node,("NULL p_node"));
			
			Mth::Vector pos;
			SkateScript::GetPosition(p_node,&pos);
			
			m_model_matrix.Ident();
			m_model_matrix.SetPos( pos + m_parent_offset * m_model_matrix );
			// m_model_matrix.SetPos(pos);
		}
		else
		{
			Mth::Vector pos_change(m_offset_of_origin_from_center[X] * m_scale3d,
								   m_offset_of_origin_from_center[Y] * m_scale3d,
								   m_offset_of_origin_from_center[Z] * m_scale3d);
			/*
			Mth::Vector pos_change(m_offset_of_origin_from_center[X] * m_summed_props.scalex * m_scale3d,
								   m_offset_of_origin_from_center[Y] * m_summed_props.scaley * m_scale3d,
								   m_offset_of_origin_from_center[Z] * m_summed_props.scalex * m_scale3d);
			*/
			sGetWorldMatrixFromScreenPosition(m_active_viewport_number, &m_model_matrix, 
											  m_summed_props.GetScreenUpperLeftX() + GetAbsW() / 2.0f, 
											  m_summed_props.GetScreenUpperLeftY() + GetAbsH() / 2.0f, 
											  m_camera_z, 
											  pos_change);
	
			// If the model were rendered using m_model_matrix as it is then it will be unscaled
			// and facing the camera.
			
			// If the object is required to point to somewhere in the world then change the rotation
			// the be the required one.
			Mth::Vector position_to_point_to;
			switch (m_point_type)
			{
			case POINT_TYPE_NODE:
				position_to_point_to=m_node_position_to_point_to;
				break;
			case POINT_TYPE_OBJECT:
			{
				// Get a pointer to the object.
				// Doing a naughty cast up to a CMovingObject, because it probably will be one, and
				// won't cause a crash if it isn't, it'll just get a bad position.
				Obj::CMovingObject *p_pos_obj=(Obj::CMovingObject*)Obj::ResolveToObject(m_id_of_object_to_point_to);
				if (p_pos_obj)
				{
					position_to_point_to=p_pos_obj->m_pos;
				}
				break;
			}	
			default:
				break;
			}
			
			if (m_point_type!=POINT_TYPE_NONE)
			{
				// Save the position
				Mth::Vector model_pos=m_model_matrix.GetPos();
				// Clear out the rotation calculated by sGetWorldMatrixFromScreenPosition
				m_model_matrix.Ident();
				
				Mth::Vector dir=position_to_point_to-model_pos;
				
				dir.Normalize();
				m_model_matrix[Z]=dir;
				
				m_model_matrix[X].Set(dir.GetZ(),0.0f,-dir.GetX());
				m_model_matrix[X].Normalize();
				
				m_model_matrix.OrthoNormalizeAbout(Z);
				
				// Tilt so that the arrow is not end on to the camera.
				// However, make the tilt drop linearly down to zero as the
				// arrow goes towards being vertical so that the arrow never
				// ends up pointing backwards.
				float r=m_tilt-(-m_model_matrix[Z].GetY()*m_tilt);
				m_model_matrix.RotateXLocal(Mth::DegToRad(r));

				// Put the position back in.
				m_model_matrix.Translate(model_pos);
			}
		}
				
		// Now create a rotation matrix containing any extra rotation that we want.
		Mth::Matrix rot_matrix;
		rot_matrix.Ident();

		rot_matrix.Translate(-m_pivot_point);

		// Update the angles using the angular velocities.
		m_angle_x+=m_angvel_x;
		m_angle_y+=m_angvel_y;
		m_angle_z+=m_angvel_z;
		
		// Reduce modulo 2pi so that the calculations don't go weird after a few days
		// due to the angles getting huge.
		m_angle_x=m_angle_x-((int)(m_angle_x/(2.0f*3.141592654f)))*(2.0f*3.141592654f);
		m_angle_y=m_angle_y-((int)(m_angle_y/(2.0f*3.141592654f)))*(2.0f*3.141592654f);
		m_angle_z=m_angle_z-((int)(m_angle_z/(2.0f*3.141592654f)))*(2.0f*3.141592654f);
		
		//rot_matrix.RotateY(m_angle_y);
		//rot_matrix.RotateX(m_angle_x);
		//rot_matrix.RotateZ(m_angle_z);
		s_apply_rotations(rot_matrix, m_angle_x, m_angle_y, m_angle_z, m_angle_order);

		rot_matrix.Translate(m_pivot_point);

		// Apply scaling.
		//Ryan("oogbat %.4f, %.4f, %.4f\n", m_summed_props.scalex, m_scale3d, m_summed_props.scalex * m_scale3d);
		Mth::Vector scale(m_summed_props.GetScaleX() * m_scale3d,
						  m_summed_props.GetScaleY() * m_scale3d,
						  m_summed_props.GetScaleX() * m_scale3d,
						  1.0f);
						  
		// The ped arrows (or stars) need to scale according to the distance from the camera so that
		// they are visible from a distance. (TT2006)
		if (m_scale_with_distance)
		{
			Gfx::Camera *p_camera = Nx::CViewportManager::sGetActiveCamera(m_active_viewport_number);
			if (p_camera)
			{
				Mth::Vector cam_offset=m_model_matrix.GetPos()-p_camera->GetPos();
				float multiplier=cam_offset.Length()*m_scale_multiplier;
				if (multiplier < 1.0f)
				{
					multiplier=1.0f;
				}
				if (multiplier > m_max_scale)
				{
					multiplier = m_max_scale;
				}
					
				scale[X]*=multiplier;
				scale[Y]*=multiplier;
				scale[Z]*=multiplier;
			}	
		}
		
		rot_matrix.Scale(scale);

		// Apply the rotation and scaling to the m_model_matrix	
		m_model_matrix=rot_matrix*m_model_matrix;


		// Calculate any hover offset that may need to be applied.		
		Mth::Vector hover_offset;
		if (m_hover_amp>0.0f)
		{
			int t=Tmr::ElapsedTime(0)%m_hover_period;
			float h=m_hover_amp*sinf(t*2*3.141592653f/m_hover_period);
			// Use m_model_matrix[Y] so that it hovers up and down in the model's
			// coordinate system.
			hover_offset=h*m_model_matrix[Y];
		}
		
		// Render the models
		bool show_models = (m_summed_props.alpha >= .0001f);
		
		// If running a replay, hide arrows that are pointing to something,
		// otherwise they go all spazzy. (TT8083 and TT8819)
		if ((m_point_type!=POINT_TYPE_NONE || m_parent_object_name) && Replay::RunningReplay())
		{
			show_models=false;
		}	
		

		for (int i=0; i<m_num_models; ++i)
		{
			Dbg_MsgAssert(mp_models[i].mpModel,("NULL mp_models[i].mpModel"));
			
			Mth::Vector p=mp_models[i].mOffset*m_model_matrix;
			Mth::Matrix display_matrix=m_model_matrix;
			
			if (m_hover_amp>0.0f)
			{
				p+=hover_offset;
			}
				
			display_matrix.SetPos(p);
			
			mp_models[i].mpModel->SetActive(show_models);
			mp_models[i].mpModel->Render(&display_matrix,false,mp_models[i].mpSkeleton);
		}
	}
}

void CElement3d::SetMorph(Script::CStruct *pProps)
{
	CScreenElement::SetMorph(pProps);
	if (pProps->ContainsFlag("scale_to_screen_dims"))
		AutoComputeScale();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CElement3d::update_visibility()
{
	// Make visible for only this viewport
	uint32 vis_mask = 1 << m_active_viewport_number;
	for (int i=0; i<m_num_models; ++i)
	{
		Dbg_MsgAssert(mp_models[i].mpModel,("NULL mp_models[i].mpModel"));

		mp_models[i].mpModel->SetVisibility(vis_mask);
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CElement3d::s_apply_rotations(Mth::Matrix & mat, float x_rot, float y_rot, float z_rot, EAngleOrder rot_order)
{
	switch (rot_order)
	{
	case ANGLE_ORDER_XYZ:
		mat.RotateX(x_rot);
		mat.RotateY(y_rot);
		mat.RotateZ(z_rot);
		break;

	case ANGLE_ORDER_XZY:
		mat.RotateX(x_rot);
		mat.RotateZ(z_rot);
		mat.RotateY(y_rot);
		break;

	case ANGLE_ORDER_YXZ:
		mat.RotateY(y_rot);
		mat.RotateX(x_rot);
		mat.RotateZ(z_rot);
		break;

	case ANGLE_ORDER_YZX:
		mat.RotateY(y_rot);
		mat.RotateZ(z_rot);
		mat.RotateX(x_rot);
		break;

	case ANGLE_ORDER_ZXY:
		mat.RotateZ(z_rot);
		mat.RotateX(x_rot);
		mat.RotateY(y_rot);
		break;

	case ANGLE_ORDER_ZYX:
		mat.RotateZ(z_rot);
		mat.RotateY(y_rot);
		mat.RotateX(x_rot);
		break;
	}
}

}
