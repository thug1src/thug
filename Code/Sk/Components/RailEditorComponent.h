//****************************************************************************
//* MODULE:         Sk/Components
//* FILENAME:       RailEditorComponent.h
//* OWNER:          Kendall Harrison
//* CREATION DATE:  6/19/2003
//****************************************************************************

#ifndef __COMPONENTS_RAILEDITORCOMPONENT_H__
#define __COMPONENTS_RAILEDITORCOMPONENT_H__

#include <core/defines.h>
#include <core/support.h>

#ifndef __SYS_MEM_POOLABLE_H
#include <sys/mem/poolable.h>
#endif

#ifndef __OBJECT_BASECOMPONENT_H__
#include <gel/object/basecomponent.h>
#endif

#ifndef __GFX_IMAGE_IMAGEBASIC_H
#include <gfx/image/imagebasic.h>
#endif

// Replace this with the CRCD of the component you are adding
#define		CRC_RAILEDITOR CRCD(0x5b509ad3,"RailEditor")

//  Standard accessor macros for getting the component either from within an object, or 
//  given an object				 
#define		GetRailEditorComponent() ((Obj::CRailEditorComponent*)GetComponent(CRC_RAILEDITOR))
#define		GetRailEditorComponentFromObject(pObj) ((Obj::CRailEditorComponent*)(pObj)->GetComponent(CRC_RAILEDITOR))

namespace Nx
{
	class CSector;
}
	
namespace Obj
{
extern Mth::Vector ZeroVector;

enum
{
	MAX_EDITED_RAIL_POINTS=400,
	
	// Single point rails are not allowed, so the maximum possible number of rails will be half the
	// number of points, since the shortest rail will have 2 points.
	MAX_EDITED_RAILS=MAX_EDITED_RAIL_POINTS/2,
};

struct SCompressedRailPoint
{
	// These are the original float values * 8
	sint16 mX,mY,mZ;
	// This is the original float height * 16
	uint16 mHeight;
	uint16 mHasPost;
};
	
class CEditedRailPoint : public Mem::CPoolable<CEditedRailPoint>
{
public:
	CEditedRailPoint();
	~CEditedRailPoint();

	void WriteCompressedRailPoint(SCompressedRailPoint *p_dest);
	
	void UpdateRailGeometry(Mth::Vector& rotateCentre=ZeroVector, Mth::Vector& newCentre=ZeroVector, float degrees=0.0f);
	void UpdatePostGeometry(Mth::Vector& rotateCentre=ZeroVector, Mth::Vector& newCentre=ZeroVector, float degrees=0.0f);
	void DestroyRailGeometry();
	void DestroyPostGeometry();
	
	float FindGroundY();
	void AdjustY();
	void InitialiseHeight();

	// These enums are just to avoid passing multiple bools to Highlight()
	enum EFlash
	{
		DONT_FLASH=0,
		FLASH=1,
	};
	enum EEndPosts
	{
		DONT_INCLUDE_END_POSTS=0,
		INCLUDE_END_POSTS=1,
	};	
	void Highlight(EFlash flash, EEndPosts includeEndPosts);
	void UnHighlight();

	void SetColor(Image::RGBA rgba);
	void ClearColor();

	void SetSectorActiveStatus(bool active);
	
	bool AngleIsOKToGrind();
		
	// This class needs to be kept as small as possible since a pool of them exists.	
	
	// MEMOPT: Change pos to be 3 sint16's, would save 10K for 400 points.
	// Used x(fixed)=int(9.0f*x(float)+0.5f) if x>0, x(fixed)=int(9.0f*x(float)-0.5f) if x<0
	Mth::Vector mPos;
	//sint16 m_x,m_y,m_z;
	
	// MEMOPT: Change to be bitfield of flags.
	bool mHasPost;
	bool mHighlighted;
	
	// The height is stored so that if the ground moves up or down, mPos[Y] can be recalculated
	// so as to maintain the post height.
	float mHeightAboveGround;

	
	CEditedRailPoint *mpNext;	
	// TODO: MEMOPT
	// Don't really need the list to be doubly linked
	CEditedRailPoint *mpPrevious;	


	// The rail geometry joining the last point to this point.
	// If this is the first point of the rail, mpClonedRailSector will be NULL.
	Nx::CSector *mpClonedRailSector;	

	// The post geometry joining this point to the ground. May be NULL, since the user chooses
	// whether they want a post.
	// Also, mpPostSector may be NULL even when mHasPost is true, because we don't always want 
	// the geometry for the rail to exist.
	// For example, when loading a park off the memory card in the skateshop it will set the mPos
	// and mHasPost members, but the sector pointers will remain NULL until it is time to build the park.
	Nx::CSector *mpPostSector;
};

class CEditedRail : public Mem::CPoolable<CEditedRail>
{
public:
	CEditedRail();
	~CEditedRail();

	void Clear();

	CEditedRail *mpNext;	
	CEditedRail *mpPrevious;
	
	CEditedRailPoint *mpRailPoints;
	
	uint32 mId;

	CEditedRailPoint *AddPoint();

	void AdjustYs();
	void InitialiseHeights();
	void UpdateRailGeometry(Mth::Vector& rotateCentre=ZeroVector, Mth::Vector& newCentre=ZeroVector, float degrees=0.0f);
	void UpdatePostGeometry(Mth::Vector& rotateCentre=ZeroVector, Mth::Vector& newCentre=ZeroVector, float degrees=0.0f);
	void DestroyRailGeometry();
	void DestroyPostGeometry();
	
	int CountPoints();
	bool FindNearestRailPoint(Mth::Vector &pos, 
							  Mth::Vector *p_nearest_pos, float *p_dist_squared, int *p_rail_point_index, 
							  int ignore_index=-1);
							  
	int CountPointsInArea(float x0, float z0, float x1, float z1);
	void DuplicateAndAddPoint(CEditedRailPoint *p_point);
	CEditedRail *GenerateDuplicateRails(float x0, float z0, float x1, float z1, CEditedRail *p_head);	
							  
	enum EUpdateSuperSectors
	{
		DONT_UPDATE_SUPER_SECTORS=0,
		UPDATE_SUPER_SECTORS=1
	};	
	bool UpdateRailPointPosition(int rail_point_index, Mth::Vector &pos, EUpdateSuperSectors updateSuperSectors);
	CEditedRailPoint *GetRailPointFromIndex(int index);
	void UnHighlight();

	CEditedRailPoint *DeleteRailPoint(CEditedRailPoint *p_point);
	
	void CopyRail(CEditedRail *p_source_rail);
	void RotateAndTranslate(Mth::Vector& rotateCentre, Mth::Vector& newCentre, float degrees);

	SCompressedRailPoint *WriteCompressedRailPoints(SCompressedRailPoint *p_dest);

	bool ThereAreRailPointsOutsideArea(float x0, float z0, float x1, float z1);

	// Used by graffiti games
	void ModulateRailColor(int seqIndex);
	void ClearRailColor();

	void SetSectorActiveStatus(bool active);
	
	void GetDebugInfo( Script::CStruct* p_info );
};

class CInputComponent;
class CEditorCameraComponent;

class CRailEditorComponent : public CBaseComponent
{
	CInputComponent 		*mp_input_component;
	CEditorCameraComponent 	*mp_editor_camera_component;
	void get_pointers_to_required_components();

	void get_pos_from_camera_component(Mth::Vector *p_pos, float *p_height, float *p_angle);
		
	int count_rails();
	
	CEditedRail *mp_edited_rails;
	CEditedRail *mp_current_rail;
	
	// The mode that the cursor is in, ie FreeRoaming, RailLayout or Grab
	// This is stored as a checksum rather than an enum because it will mostly
	// be scripts that are doing logic based on the mode, and this way I don't
	// have to define a bunch of script global integers to match the enum.
	uint32 m_mode;
	
	uint32 m_dotted_line_sector_name;	
	Nx::CSector *mp_dotted_line_sector;

	enum
	{
		COMPRESSED_RAILS_BUFFER_SIZE = sizeof(uint16) + // Num rails
									   MAX_EDITED_RAILS * sizeof(uint16) + // Num points in each rail
									   MAX_EDITED_RAIL_POINTS * sizeof(SCompressedRailPoint), // The rail points
	};									   
		
	uint8 mp_compressed_rails_buffer[COMPRESSED_RAILS_BUFFER_SIZE];
	void generate_compressed_rails_buffer();
	void clear_compressed_rails_buffer();
	
	// Used by graffiti games
	CEditedRail *get_rail_from_cluster_name(uint32 clusterChecksum);
	
public:
    CRailEditorComponent();
    virtual ~CRailEditorComponent();

public:
	uint8 *							GetCompressedRailsBuffer();
	int								GetCompressedRailsBufferSize() {return COMPRESSED_RAILS_BUFFER_SIZE;}
	void							SetCompressedRailsBuffer(uint8 *p_buffer);
	void							InitUsingCompressedRailsBuffer();
	
	void							Clear();
	CEditedRail *					NewRail();
	bool							NewRail(const char *p_railName);
	void							DeleteRail(CEditedRail *p_rail);
	CEditedRail *					GetRail(uint32 id);
	void							RemoveEmptyAndSinglePointRails();
	
	int								GetTotalRailNodesRequired();
	void							SetUpRailNodes(Script::CArray *p_nodeArray, int *p_nodeNum, uint32 firstID);

	void							AdjustYs();
	void							InitialiseHeights();
	void							UpdateRailGeometry();
	void							UpdatePostGeometry();
	void							DestroyRailGeometry();
	void							DestroyPostGeometry();

	void							RefreshGeometry();
	void							UnHighlightAllRails();
	
	bool							DeleteRailPoint(CEditedRail *p_rail, CEditedRailPoint *p_point);

	enum EClipType
	{
		DELETE_POINTS_OUTSIDE=0,
		DELETE_POINTS_INSIDE=1
	};
	bool							ClipRail(CEditedRail *p_rail, float x0, float z0, float x1, float z1, EClipType clipType);
	void							ClipRails(float x0, float z0, float x1, float z1, EClipType clipType);
	bool							ThereAreRailPointsOutsideArea(float x0, float z0, float x1, float z1);
	

	int								CountPointsInArea(float x0, float z0, float x1, float z1);
	bool							AbleToCopyRails(float x0, float z0, float x1, float z1);
	CEditedRail *					GenerateDuplicateRails(float x0, float z0, float x1, float z1);
									
	bool							FindNearestRailPoint(Mth::Vector &pos, Mth::Vector *p_nearest_pos, 
														 float *p_dist_squared, 
														 uint32 *p_rail_id, int *p_rail_point_index,
														 uint32 ignore_rail_index=0, int ignore_rail_point_index=-1);

	
	void							DeleteDottedLine();
	void							DrawDottedLine(Mth::Vector& pos);

	int								GetNumFreePoints();
	int								GetNumFreeRails();
	
	// Used by graffiti games
	void							ModulateRailColor(uint32 clusterChecksum, int seqIndex);
	void							ClearRailColor(uint32 clusterChecksum);

	void							SetSectorActiveStatus(bool active);
	
	// A quick hack to allow UpdateSuperSectors not to be called when loading a new park in a net game.
	// If it is called, the whole park disappears due to all the sectors having been flagged for deletion.
	static bool						sUpdateSuperSectorsAfterDeletingRailSectors;
	
	// For reading/writing to memcard
	void							WriteIntoStructure(Script::CStruct *p_info);
	void							ReadFromStructure(Script::CStruct *p_info);

	virtual	void 					Finalize();
    virtual void            		Update();
    virtual void            		InitFromStructure( Script::CStruct* pParams );
    virtual void            		RefreshFromStructure( Script::CStruct* pParams );
    
    virtual EMemberFunctionResult   CallMemberFunction( uint32 Checksum, Script::CStruct* pParams, Script::CScript* pScript );
	virtual void 					GetDebugInfo( Script::CStruct* p_info );
	virtual void					Hide( bool shouldHide );

	static CBaseComponent*			s_create();
};

Obj::CRailEditorComponent *GetRailEditor();


// This will print the memory used by the rail editor component, via a compiler error.
// Need to keep it within 30K, since it exists in memory permanently.
//char p[(sizeof(CRailEditorComponent)+MAX_EDITED_RAIL_POINTS*sizeof(CEditedRailPoint)+MAX_EDITED_RAILS*sizeof(CEditedRail))/0];

} // namespace Obj

#endif // #ifndef __COMPONENTS_RAILEDITORCOMPONENT_H__
