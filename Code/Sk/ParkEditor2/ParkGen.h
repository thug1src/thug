#ifndef __SK_PARKEDITOR2_PARKGEN_H
#define __SK_PARKEDITOR2_PARKGEN_H

#include <core/math/vector.h>
#include <sk/parkeditor/edrail.h>

#define DEBUG_THIS_DAMN_THING 0

#if defined ( __PLAT_XBOX__ ) || defined ( __PLAT_WN32__ )
	inline void ParkEd(const char* A ...) {};
#else

	#if DEBUG_THIS_DAMN_THING
		#define ParkEd(A...)  printf("PARKED: "); printf(##A); printf("\n") 		
	#else
		#define ParkEd(A...)
	#endif
#endif

namespace Image
{
	struct RGBA;
}

namespace Nx
{
	class CScene;
	class CSector;
}

namespace Script
{
	class	CArray;
}

namespace Ed
{

	class CSourcePiece;
	class CClonedPiece;
	class CMapListNode;

struct MapArea		 	// Mick - OLD
{
	int x, y, z;
	int w, h, l;
};


enum
{
	H = 4,
	L = 5
};




/*
	Describes a 3D box in cell coordinates. Sometimes only the position part or only the area part is used.
*/
class GridDims
{
public:
								GridDims(uint8 x = 0, sint8 y = 0, uint8 z = 0, uint8 w = 0, uint8 h = 0, uint8 l = 0);

	void						SetXYZ(uint8 x, sint8 y, uint8 z) {m_dims[0] = x; m_dims[1] = (uint8) y; m_dims[2] = z;}
	void						SetWHL(uint8 w, uint8 h, uint8 l) {m_dims[3] = w; m_dims[4] = h; m_dims[5] = l;}

	uint8 &						operator[](sint i);
	
	uint8						GetX() const {return m_dims[0];}
	sint8						GetY() const {return (sint8) m_dims[1];}
	uint8						GetZ() const {return m_dims[2];}
	uint8						GetW() const {return m_dims[3];}
	uint8						GetH() const {return m_dims[4];}
	uint8						GetL() const {return m_dims[5];}

	void						MakeInfinitelyHigh();
	void						MakeInfiniteOnY();

	void						PrintContents() const;

private:

	uint8						m_dims[6];
};


/*
	Roles of CPiece:
	-wraps object placed in park
	-wraps fluid object contained in world (cursor)
	
	Class hierarchy:
	-CPiece
		-CSourcePiece (dims)
		-CClonedPiece (pos, rot, pointer to master)
		
	Rotations:				  
	-Hard: underlying verts are flipped around directly, stored dims changed to match
	-Soft: matrix sent down directly, not stored with piece
	
	Relationship with metapiece:
	-pieces contained by metapiece must be of the same type, all hard pieces or all soft pieces
*/
class CPiece
{
	friend class CParkGenerator;
	friend class CMetaPiece;
	friend class CConcreteMetaPiece;

public:

	enum EFlags
	{
		mNO_FLAGS =					0,

		// only one of following can be set
		mSOURCE_PIECE =				(1<<0),
		mCLONED_PIECE =				(1<<1),
		
		// Applied to a cloned piece. Set if an animated piece, like part of cursor
		mSOFT_PIECE =				(1<<2),
		
		// set if piece is being displayed
		mIN_WORLD =					(1<<3),

		mSHELL = 					(1<<4),
		mFLOOR = 					(1<<5),
		mGAP = 						(1<<6),
		mRESTART =					(1<<7),
		mNO_RAILS =					(1<<8),
	};

	EFlags							GetFlags() {return m_flags;}
	virtual Mth::Vector				GetDims() = 0;
	void							GetCellDims(GridDims *pDims);

	CSourcePiece *					CastToCSourcePiece();
	CClonedPiece *					CastToCClonedPiece();

	Mth::Vector						GetDimsWithRot(Mth::ERot90 rot);

protected:

	/*
		Hard rotation: flipping of verts
		
		Soft: rotation of underlying piece, matrix is set for CSector directly.
	*/									
	
									CPiece();
	virtual							~CPiece();

	void							set_flag(EFlags flag) {m_flags = EFlags(m_flags | flag);}
	void							clear_flag(EFlags flag) {m_flags = EFlags(m_flags & ~flag);}

	union
	{
		uint32						m_sector_checksum;		// probably will refer to CSectors this way
		// 							mp_other_thing // for soft pieces
	};

	EFlags							m_flags;

	CPiece *						mp_next_in_list;
	//CPiece *						mp_next_in_metapiece;
};




class CSourcePiece : public CPiece
{
	friend class CParkGenerator;
	friend class CMetaPiece;
	friend class CClonedPiece;

public:
	/*
		pieces in different domains can coexist at same GRID coordinates
	*/
	enum EDomain
	{
		mREGULAR				= (1<<0),
		mOFFSET_RAIL			= (1<<1),
		mGAP					= (1<<2),
	};

	uint32							GetType();
	Mth::Vector						GetDims() {return m_dims;}

protected:
									CSourcePiece();
									~CSourcePiece();

	Mth::Vector						m_dims;
	EDomain							m_domain;	
	uint32							m_triggerScriptId;  // Checksum of trigger script associated with this node (for stuff like chainlink fence SFX)

	int								m_num_rail_points;
	int								m_num_linked_rail_points;
};




class CClonedPiece : public CPiece
{
	friend class CParkGenerator;
	friend class CMetaPiece;
	friend class CConcreteMetaPiece;

public:
	uint32							GetID();
	
	enum ESectorEffect
	{
		MARKER_ONLY,
		CHANGE_SECTOR,
	};
	void							SetDesiredPos(const Mth::Vector& pos, ESectorEffect sectorEffect);
	void							SetDesiredRot(Mth::ERot90 rot, ESectorEffect sectorEffect);
	void							SetSoftRot(float rot); // in degrees
	void							SetScaleX(float scaleX);
	void							Highlight(bool on, bool makePurple = false);
	void							SetActive(bool active);
	void							SetVisibility(bool visible);
	CSourcePiece *					GetSourcePiece() {return mp_source_piece;}
	Mth::Vector						GetPos() {return m_pos;}
	Mth::ERot90						GetRot() {return m_rot;}
	Mth::Vector						GetDims();
	uint32							GetType() {return mp_source_piece->GetType();}

protected:
									CClonedPiece();

	uint32							m_id;
	CSourcePiece *					mp_source_piece;

	Mth::Vector						m_pos; // center, bottom
	Mth::ERot90						m_rot; // in units of 90 degrees, CCW
};




class CParkGenerator
{

public:

	static const float 			CELL_WIDTH;
	static const float 			CELL_HEIGHT;
	static const float 			CELL_LENGTH;


	static const uint32				vFIRST_ID_FOR_OBJECTS;
	static const uint32				vMAX_ID_FOR_OBJECTS;
	static const uint32				vFIRST_ID_FOR_RAILS;
	static const uint32				vMAX_ID_FOR_RAILS;
	static const uint32				vFIRST_ID_FOR_CREATED_RAILS;
	static const uint32				vMAX_ID_FOR_CREATED_RAILS;
	static const uint32				vFIRST_ID_FOR_RESTARTS;

	
	enum EDestroyType
	{
		DESTROY_PIECES_AND_SECTORS,
		DESTROY_ONLY_PIECES,
	};

	enum RestartType
	{
		vEMPTY,
		vONE_PLAYER,
		vMULTIPLAYER,
		vHORSE,
		vKING_OF_HILL,
		vRED_FLAG,
		vGREEN_FLAG,
		vBLUE_FLAG,
		vYELLOW_FLAG,
		
		
		vNUM_RESTART_TYPES,
	};


									CParkGenerator();
									~CParkGenerator();

	int								GetResourceSize(char *name);
	Mem::Heap *						GetParkEditorHeap();
	
	struct MemUsageInfo
	{
		int							mParkHeapUsed;
		int							mParkHeapFree; // after padding is deducted
		int							mMainHeapUsed;
		int							mMainHeapFree; // after padding is deducted
		
		int							mLastMainUsed;
		bool						mIsFragmented;

		int							mTotalRailPoints;
		int							mTotalLinkedRailPoints;
		int							mTotalClonedPieces;
	};
	MemUsageInfo					GetResourceUsageInfo(bool printInfo = false);
	void							SetMaxPlayers(int maxPlayers);
	int 							GetMaxPlayers() {return m_max_players;}
	int 							GetMaxPlayersPossible();
		
	CSourcePiece *					GetMasterPiece(uint32 pieceType, bool assert = false);
	CSourcePiece *					GetNextMasterPiece(CSourcePiece *pLast);
	CClonedPiece *					ClonePiece(CPiece *pMasterPiece, CPiece::EFlags flags);
	void							AddClonedPieceToWorld(CClonedPiece *pPiece);
	void							DestroyClonedPiece(CClonedPiece *pPiece);


	void							InitializeMasterPieces(int parkW, int parkH, int parkL, int theme);
	
	// K: Added to allow cleanup of the park editor heap during play
	//void							DeleteSourceAndClonedPieces();
	//void							DeleteParkEditorHeap();
	
	void							UnloadMasterPieces();
	
	void							PostGenerate();
	void							GenerateCollisionInfo(bool assert = true);
	void 							RemoveOuterShellPieces(int theme);
	void							GenerateNodeInfo(CMapListNode * p_concrete_metapiece_list);
	void 							ReadInRailInfo();
	void							DestroyRailInfo();

	void							HighlightAllPieces(bool highlight);
	void							SetGapPiecesVisible(bool visible);
	void							SetRestartPiecesVisible(bool visible);


	void 							SetLightProps(int num_lights, 
												  float amb_const_r, float amb_const_g, float amb_const_b, 
												  float falloff_const_r, float falloff_const_g, float falloff_const_b,
												  float cursor_ambience);
	void 							SetLight(Mth::Vector &light_pos, int light_num);
	void 							CalculateLighting(CClonedPiece *p_piece);
	void 							CalculateVertexLighting(const Mth::Vector & vert, Image::RGBA & color);
	
	
	void							DestroyAllClonedPieces(EDestroyType type);


	int								NumRestartsOfType(RestartType type);
	bool							NotEnoughRestartsOfType(RestartType type, int need);
	Mth::Vector	 					GetRestartPos(RestartType type, int index);
	GridDims	 					GetRestartDims(RestartType type, int index);
	bool							AddRestart(Mth::Vector &pos, int dir, GridDims &dims, RestartType type, bool automatic, bool auto_copy=false);
	void							RemoveRestart(const GridDims &dims, RestartType type);
	bool 							FreeRestartExists(RestartType type);
	void							KillRestarts();	
	void							ClearAutomaticRestarts();

	
	CClonedPiece *					CreateGapPiece(Mth::Vector &pos, float length, int rot);

	Nx::CScene *					GetClonedScene() {return mp_cloned_scene;}

	void							CleanUpOutRailSet();
	void							GenerateOutRailSet(CMapListNode * p_concrete_metapiece_list);
	bool							FindNearestRailPoint(Mth::Vector &pos, Mth::Vector *p_nearest_pos, float *p_dist_squared);
	
private:

	enum EFlags
	{
		mMASTER_STUFF_LOADED		= (1<<1),
		mSECTORS_GENERATED			= (1<<2),
		mPIECES_IN_WORLD			= (1<<3),
		mCOLLISION_INFO_GENERATED	= (1<<4),
		mNODE_INFO_GENERATED		= (1<<5),
		// set when we destroy a piece
		mSECTOR_MEMORY_NEEDS_FREE	= (1<<6),
	};

	void 							destroy_piece_impl(CClonedPiece *pPiece, EDestroyType destroyType);
	bool							flag_on(EFlags flag) {return ((m_flags & flag) != 0);}
	void							set_flag(EFlags flag) {m_flags = EFlags(m_flags | flag);}
	void							clear_flag(EFlags flag) {m_flags = EFlags(m_flags & ~flag);}
	
	uint32 							scan_for_cluster(Script::CArray *pNodeArray, int &index);
	int 							scan_for_rail_node(Script::CArray *pNodeArray, uint32 cluster, int link, Mth::Vector *pVector, bool *pHasLinks, RailPoint::RailType *pType);
	void							scan_in_trigger_info(Script::CArray *pNodeArray);
	void 							set_up_object_nodes(Script::CArray *pNodeArray, int *pNodeNum);
	void							set_up_rail_nodes(Script::CArray *pNodeArray, int *pNodeNum);
	void							set_up_restart_nodes(Script::CArray *pNodeArray, int *pNodeNum);

	
	/*
	Things
	
		-loading master geometry, loading master rails (one operation)
		-generating world from compressed map
		-collision setup
		-rail setup
		-cleanup world (maybe just destroys pieces)
	*/
	
	EFlags							m_flags;

	CPiece *						mp_cloned_piece_list;
	CPiece *						mp_source_piece_list;
	int								m_num_cloned_pieces;
	int								m_num_source_pieces;

	Nx::CScene *					mp_source_scene;				// Scene used as a manager of pieces
	Nx::CScene *					mp_cloned_scene;

	uint32							m_next_id;
	
	Mem::Region*					mp_mem_region;
	Mem::Heap*						mp_mem_heap;

	int								m_max_players;
		
//////////////////////////////////////////////////////////////////////////
// rail and node stuff patched in by Mick

	RailSet							m_in_rail_set;
	RailSet							m_out_rail_set;
	// in world, at editing time
	int								m_total_rail_points;
	int								m_total_rail_linked_points;

	struct TempNodeInfo
	{
		uint32						classCrc;
		uint32						cluster;
		Script::CArray *			pLinkArray;
	};

	TempNodeInfo *					m_temp_node_tab;
	int	*							m_processed_node_tab;
	int								m_processed_node_tab_entries;

	/*
		Restart stuff
	*/



	void 				get_restart_slots(RestartType type, int *pFirstSlot, int *pLastSlot);


	struct RestartInfo
	{
		Mth::Vector					pos;
		int							dir;
		GridDims					dims;
		RestartType					type;
		bool						automatic;
	};
	static const int				vNUM_RESTARTS = 18;
	
	RestartInfo						m_restart_tab[vNUM_RESTARTS];

	/*
		Light info
	*/
	
	struct LightIntensity
	{
		float r;
		float g;
		float b;
	};
	
	static const int				vMAX_LIGHTS = 16;
	
	int								m_numLights;
	LightIntensity					m_ambientLightIntensity;
	LightIntensity					m_falloffLightIntensity;
	Mth::Vector						m_lightTab[vMAX_LIGHTS];
	float							m_cursorAmbience;

	
	MemUsageInfo					m_mem_usage_info;
};




} // end namespace


#endif
