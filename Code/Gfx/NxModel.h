//****************************************************************************
//* MODULE:         Gfx
//* FILENAME:       NxModel.h
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  12/21/2001
//****************************************************************************

#ifndef	__GFX_NXMODEL_H__
#define	__GFX_NXMODEL_H__

                           
#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <core/math.h>

// Forward declarations
namespace Gfx
{
	class	CFaceTexture;
    class   CSkeleton;
};

namespace Script
{
	class	CStruct;
};
                  
namespace Nx
{

// Forward declarations
class	CGeom;
class	CHierarchyObject;
class	CMesh;
class	CTexDict;
class	CModelLights;

// TODO:  Insert class description here...
// This is the machine-independent representation of a model,
// which is the graphical representation of some object

enum ERenderMode
{
    vTEXTURED,
	vSKELETON,
    vGOURAUD,
    vFLAT,
    vWIREFRAME,
    vBBOX,
    vNONE
};

class CModel : public Spt::Class
{
public:
	enum
	{
		// special flag for global texture replacement
		vREPLACE_GLOBALLY = 0xc4e78e22			// all
	};

public:
    // The basic interface to the model
    // this is the machine independent part	
    // machine independent range checking, etc can go here	
	CModel();
    virtual				~CModel();

public:
	bool				SetBoneMatrixData( Gfx::CSkeleton* pSkeleton );
    bool                Render( Mth::Matrix* pMatrix, bool no_anim, Gfx::CSkeleton* pSkeleton );
    uint32				GetFileName() { return m_primaryMeshName; }

    // Grabs a specific geom object so that you can do part-specific operations
    CGeom*     			GetGeom(uint32 geomName);
	CGeom*				GetGeomByIndex(int index);
	uint32				GetGeomNameByIndex(int index);
	CTexDict*			GetTexDictByIndex(int index);
	int					GetNumGeoms() const;
	void				ClearGeoms();
	void				Finalize();
	uint32				GetGeomActiveMask() const;
	void				SetGeomActiveMask(uint32 mask);
	
	bool				AddGeom(uint32 assetName, uint32 geomName, bool supportMultipleMaterialColors=false);
	bool				AddGeom(const char* pMeshName, uint32 geomName, bool useAssetManager, uint32 texDictOffset=0, bool forceTexDictLookup=false, bool supportMultipleMaterialColors=false);
	bool				HideGeom( uint32 geomName, bool hidden );
    bool				GeomHidden( uint32 geomName );

	// probably need to pass the collision as well
	bool				AddGeom(Nx::CGeom* pGeom, uint32 geomName);

	// model lights
	bool				CreateModelLights();
	bool				DestroyModelLights();
	CModelLights *		GetModelLights() const;

public:
    bool                SetRenderMode(ERenderMode mode);
	ERenderMode			GetRenderMode();
	bool				SetColor(uint8 r, uint8 g, uint8 b, uint8 a);
	bool				SetVisibility(uint32 mask);
	void				ResetScale();
	bool				SetScale(const Mth::Vector& scale);
	Mth::Vector			GetScale() {return m_scale;}
	void				EnableScaling( bool enabled );
	bool				IsScalingEnabled() { return m_scalingEnabled; }
	void				Hide( bool should_hide = true );
	bool				IsHidden() { return m_hidden; }
	bool				SetActive(bool active);
	bool				GetActive() {return m_active && !m_hidden;}    
	bool                ReplaceTexture( uint32 geomName, const char* p_SrcFileName, const char* p_DstFileName );
	bool                ReplaceTexture( uint32 geomName, const char* p_SrcFileName, uint32 DstChecksum );
	bool				SetUVOffset(uint32 mat_checksum, int pass, float u_offset, float v_offset);
	bool				SetUVMatrix(uint32 mat_checksum, int pass, const Mth::Matrix &mat);
	bool				AllocateUVMatrixParams(uint32 mat_checksum, int pass);
    bool                SetSkeleton( Gfx::CSkeleton* pSkeleton );
    Mth::Matrix*		GetBoneTransforms();
    void				EnableShadow(bool enabled);
    bool				RemovePolys();
	uint32 				GetPolyRemovalMask();
	void 				HidePolys( uint32 polyRemovalMask );

	// way of changing object color (for general objects)
	bool				ModulateColor( uint32 geomName, float h, float s, float v );
	bool				ClearColor( uint32 geomName );
	
	// generic way of changing colors, without having to know whether
	// you've got material colors or object colors...  (used from
	// model builder)
	bool				SetColor( Script::CStruct* pParams, float h, float s, float v );
	bool				ClearColor( Script::CStruct* pParams );
	
	// apply face texture (kind of like a texture replacement)
	bool				ApplyFaceTexture( Gfx::CFaceTexture* pFaceTexture, const char* pSrcTexture, uint32 geomName );

    // phase this out, eventually...
    int                 GetNumBones() {return m_numBones;}      // not really happy about this

protected:
    ERenderMode         m_renderMode;
    
protected:
	// GJ:  skeleton related stuff...  I'm not sure
	// if this is the appropriate place to store this
	// data...  maybe model should be able to query the 
	// composite object for this data?
	int                 m_numBones;
    Mth::Matrix*        mp_skeletonMatrices;

public:
	Mth::Vector			GetBoundingSphere();
	void				SetBoundingSphere( const Mth::Vector& boundingSphere );

	// for skeletal models, such as cars
	Nx::CHierarchyObject* GetHierarchy();
	int					GetNumObjectsInHierarchy();

	void				EnableShadowVolume(bool enabled);

protected:
	// TODO:  replace this with a linked list
	enum
	{
		MAX_GEOMS = 18
	};
	CGeom*				mp_geom[MAX_GEOMS];
	Nx::CTexDict*		mp_geomTexDict[MAX_GEOMS];
	uint32				m_geomName[MAX_GEOMS];
	uint32				m_geomActiveMask;
	Mth::Vector			m_scale;

	CModelLights*		mp_model_lights;

	enum
	{
		MAX_MESHES = 18
	};

	CMesh*				mp_mesh[MAX_MESHES];
	uint32				m_preloadedMeshNames[MAX_MESHES];
	uint32				m_primaryMeshName;

	// remember the bounding sphere
	Mth::Vector			m_boundingSphere;
	
	bool				m_boundingSphereCached:1;
	bool				m_active:1;
	bool				m_hidden:1;
	bool				m_scalingEnabled:1;
	bool				m_shadowEnabled:1;
	bool				m_doShadowVolume:1;

	short				m_numGeoms;
	short				m_numMeshes;
	short				m_numPreloadedMeshes;
	
private:
    // The virtual functions will have a stub implementation
    // in p_nxmodel.cpp
	virtual	bool		plat_init_skeleton( int numBones );
	virtual	bool		plat_set_render_mode(ERenderMode mode);
	virtual	bool		plat_set_color(uint8 r, uint8 g, uint8 b, uint8 a);
	virtual	bool		plat_set_visibility(uint32 mask);
	virtual	bool		plat_set_active(bool active);
    virtual bool        plat_set_scale(float scaleFactor);
    virtual bool        plat_replace_texture(char* p_srcFileName, char* p_dstFileName);
	virtual bool		plat_render(Mth::Matrix* pRootMatrix, Mth::Matrix* ppBoneMatrices, int numBones);
	virtual bool		plat_prepare_materials( void );
	virtual bool		plat_refresh_materials( void );
	virtual Mth::Vector	plat_get_bounding_sphere();
	virtual void		plat_set_bounding_sphere( const Mth::Vector& boundingSphere );
};

}

#endif // __GFX_NXMODEL_H__

