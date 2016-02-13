#ifndef __SK_PARKEDITOR2_EDMAP_H
#define __SK_PARKEDITOR2_EDMAP_H

#include <sk/ParkEditor2/ParkGen.h>
#include <sk/ParkEditor2/GapManager.h>

#ifndef __SYS_MEM_POOLABLE_H
#include <sys/mem/poolable.h>
#endif

//#ifdef	__PLAT_NGPS__
#define USE_BUILD_LIST
//#endif


namespace Script
{
	class CStruct;
	class CScript;
}


namespace Ed
{

class CPiece;
class CSourcePiece;
class CClonedPiece;
class CParkGenerator;
class CConcreteMetaPiece;
class CAbstractMetaPiece;

/*
	Describes how to incorporate a CPiece or hardcoded metapiece into a CMetaPiece.
	
	The position is relative to the NW corner of the piece, in its current rotation.
	
	Likewise, the rotation value includes the rotation of the whole metapiece.
*/
struct SMetaDescriptor
{
	uint8						mX;
	uint8						mY;
	uint8						mZ;
	uint8						mRot;
	uint8						mRiser; // true if a riser
	
	/* 
		The piece pointer is active within a concrete metapiece, the name in an abstract one.
		In the latter case, the name refers to a metapiece.
	*/
	union
	{
		uint32					mPieceName;
		CPiece *				mpPiece;
	};
};




/*
	What is a metapiece?
	
	-Wraps a group of cloned pieces that are in the park (or about to be)
	
	-Wraps a group of cloned pieces that make up the cursor. Contains extra position,
		rotation info.
	
	-Wraps a group of cloned pieces that make up a menu item. Refers to a 3D element.
		
	-Wraps a descriptor table that tells how to build a full metapiece.
	
	Rules:
	-If metapiece is in world, positions of contained pieces are in world
	-Otherwise, positions are relative to center bottom of metapiece
	
	Other notes:
	-An abstract user-created metapiece can contain hard-coded metapieces
	-A concrete metapiece cannot
*/
class CMetaPiece
{
	friend class CParkManager;

public:

	enum EFlags
	{
		mCONCRETE_META			= (1<<0),
		mABSTRACT_META			= (1<<1),

		mIN_PARK				= (1<<2),

		mIS_RISER				= (1<<3),
		mMARK_FOR_KILL			= (1<<4),
		mMARK_AS_SLID			= (1<<5),

		mUSER_CREATED			= (1<<6),
		mSINGULAR				= (1<<7),

		mINNER_SHELL			= (1<<8),

		mLOCK_AGAINST_DELETE	= (1<<9),
		mNO_RAILS				= (1<<10),

		mGAP_PLACEMENT			= (1<<11),
		mGAP_ATTACHED			= (1<<12),

		mNOT_ON_GC				= (1<<13), // this metapiece does not appear on Gamecube

		mCONTAINS_RISERS		= (1<<14),
		mAREA_SELECTION			= (1<<15),
		
		mRAIL_PLACEMENT			= (1<<16),
	};

	CConcreteMetaPiece *		CastToConcreteMeta();
	CAbstractMetaPiece *		CastToAbstractMeta();

	bool						IsRestartOrFlag();
	bool						IsCompletelyWithinArea(const GridDims &area);
	
	
	virtual CPiece *			GetContainedPiece(int index) = 0;

	void						SetRot(Mth::ERot90 rot);
	Mth::Vector 				GetRelativePosOfContainedPiece(int index);
	void 						BuildElement3dSectorsArray(Script::CArray *pArray);

	/*
		Returns cell dimensions occupied by metapiece (post-rotation). Coordinates in GridDims structure
		are not valid in an abstract metapiece, only in a concrete one that's in the park.
	*/
	const GridDims &			GetArea() {return m_cell_area;}
	uint32						GetNameChecksum() {return m_name_checksum;}
	// Returns rotation of whole metapiece
	Mth::ERot90					GetRot() {return Mth::ERot90(m_rot);}

	bool						IsInPark() {return m_flags & mIN_PARK;}
	// Returns true if the WHOLE metapiece is a riser
	bool 						IsRiser() {return m_flags & mIS_RISER;}
	bool						IsInnerShell() {return m_flags & mINNER_SHELL;}
	bool						IsSingular() {return m_flags & mSINGULAR;}
	bool 						NoRails() {return m_flags & mNO_RAILS;}
	bool						IsGapPlacement() {return m_flags & mGAP_PLACEMENT;}
	bool						IsAreaSelection() {return m_flags & mAREA_SELECTION;}
	bool						IsRailPlacement() {return m_flags & mRAIL_PLACEMENT;}
	bool 						IsConcrete() {return m_flags & mCONCRETE_META;}

	void						MarkAsInnerShell() {m_flags = EFlags(m_flags | mINNER_SHELL);}
	void						MarkLocked() {m_flags = EFlags(m_flags | mLOCK_AGAINST_DELETE);}
	void						MarkUnlocked() {m_flags = EFlags(m_flags & ~mLOCK_AGAINST_DELETE);}
	

protected:
								CMetaPiece();
	virtual						~CMetaPiece();

	void						initialize_desc_table(int numEntries);
	SMetaDescriptor &			get_desc_at_index(int i);

	/*
		Adds a CPiece to the metapiece. If the metapiece is abstract, the CPiece must be a CSourcePiece. If the metapiece is
		concrete, the CPiece must be a CClonedPiece. 
		
		pPos specifies the position of the pPiece relative to the northwest corner of the metapiece.
		
		isRiser set if pPiece is meant to act as a riser.
	*/
	virtual	void				add_piece(CPiece *pPiece, GridDims *pPos, Mth::ERot90 rot = Mth::ROT_0, bool isRiser = false) = 0;
	
	void						set_flag(EFlags flag) {m_flags = EFlags(m_flags | flag);}
	void						clear_flag(EFlags flag) {m_flags = EFlags(m_flags & ~flag);}
	
	// takes into account current rotation
	GridDims					m_cell_area;			//  6 ( 8) + 2
									
	EFlags 						m_flags;   				// 	4 (only uses 17 bits, grr)
	int							m_rot;					//  4 (only uses  2 bits, grr)   +4 (if combined)
	
	/* 
		-See comment above SMetaDescriptor declaration for details on what descriptors are
		-Most metapieces will only contain one piece, so mp_additional_desc_tab is only used if there are more
	*/

	enum
	{
		USUAL_NUM_DESCRIPTORS =	2
	};
	
	SMetaDescriptor 			m_first_desc_tab[USUAL_NUM_DESCRIPTORS];  //  9 (12), + 9 (12)	  + 6
	SMetaDescriptor *			mp_additional_desc_tab;	  				  //  4	
	uint						m_total_entries;						  //  2 (4) 			  + 2
	uint 						m_num_used_entries;						  //  2 (4) 			  +2
	
	uint32						m_name_checksum; 						  //  4
};

// Total 													



class CConcreteMetaPiece : public CMetaPiece
{
	friend class CParkManager;

public:
	
	int							CountContainedPieces();
	CPiece *					GetContainedPiece(int index);

	void						SetSoftRot(Mth::Vector pos, float rot);

	void						Highlight(bool on, bool gapHighlight = false);
	void						SetVisibility(bool visible);

protected:
	
								CConcreteMetaPiece();
	
	void						lock();
	void						get_max_min_world_coords(Mth::Vector &max, Mth::Vector &min);
	
	bool 						test_for_conflict(GridDims area, bool excludeRisers);
	
	// see comments with original declaration
	virtual void				add_piece(CPiece *pPiece, GridDims *pPos, Mth::ERot90 rot = Mth::ROT_0, bool isRiser = false);
};




class CAbstractMetaPiece : public CMetaPiece
{
	friend class CParkManager;

public:
protected:
								CAbstractMetaPiece();

	CPiece *					GetContainedPiece(int index);
	
	// see comments with original declaration
	virtual void				add_piece(CPiece *pPiece, GridDims *pPos, Mth::ERot90 rot = Mth::ROT_0, bool isRiser = false);
	void 						add_piece_dumb(CAbstractMetaPiece *pMeta, GridDims *pPos, Mth::ERot90 rot, bool isRiser);

	SMetaDescriptor 			get_desc_with_expansion(int index);
	int 						count_descriptors_expanded();
};




class CMapListNode : public Mem::CPoolable<CMapListNode>
{
	friend class CParkManager;
	friend class CCursor;

public:
	CMetaPiece *				GetMeta();
	CConcreteMetaPiece *		GetConcreteMeta() {return GetMeta()->CastToConcreteMeta();}
	CAbstractMetaPiece *		GetAbstractMeta() {return GetMeta()->CastToAbstractMeta();}
	CMapListNode *				GetNext() {return mp_next;}
	void						DestroyList();
	void						VerifyIntegrity();

	int							mSlideAmount;

private:
	CMetaPiece *				mp_meta;
	CMapListNode *				mp_next;
};




class CMapListTemp
{
public:
	CMapListTemp(CMapListNode *p_list);
	~CMapListTemp();
	
	bool						IsEmpty() {return (mp_list == NULL);}
	bool						IsSingular() {return (mp_list->GetNext() == NULL);}
	CMapListNode *				GetList();
	void						PrintContents();

private:

	CMapListNode *				mp_list;
};




class CParkManager
{
	DeclareSingletonClass( CParkManager );

public:

	enum
	{
		MAX_WIDTH =				56,
		MAX_LENGTH =			56,
		MAX_HEIGHT =			16,
		/* 
			These next two account for the border around the whole park. Riser pieces must be added when the user
			digs a hole at the edge of the park.
		*/
		FLOOR_MAX_WIDTH =		MAX_WIDTH + 2,
		FLOOR_MAX_LENGTH =		MAX_LENGTH + 2,

		COMPRESSED_MAP_SIZE		= 15000,		// (Mick) it can acutally be bigger than this, but I've made it this size so network code won't crash
	};

								CParkManager();
								~CParkManager();

	/* ======== General Functions ======== */

	CParkGenerator *			GetGenerator() {return mp_generator;}								
	void						Initialize();
	bool						IsInitialized() {return m_state_on;}
	void						AccessDisk(bool save, int fileSlot);
	void						Destroy(CParkGenerator::EDestroyType type);

	float						GetClipboardProportionUsed();
	void						WriteCompressedMapBuffer();
	uint8*						GetCompressedMapBuffer(bool markSaved = false);
	void						SetCompressedMapBuffer(uint8* src_buffer, bool markNotSaved = false);
	bool						IsMapSavedLocally();
	void						WriteIntoStructure(Script::CStruct *p_struct);
	#ifdef __PLAT_NGC__
	void 						ReadFromStructure(Script::CStruct *p_struct, bool do_post_mem_card_load=true, bool preMadePark=false);
	#else
	void						ReadFromStructure(Script::CStruct *p_struct, bool do_post_mem_card_load=true);
	#endif
	
	int							GetTheme();
	void						SetTheme(int theme);
	uint32						GetShellSceneID();
	const char *				GetParkName();
	void						SetParkName(const char *pName);
	uint32						GetParkChecksum();
	int							GetCompressedParkWidth();
	int							GetCompressedParkLength();

	/* ======== Rebuild functions ======== */

	void						RebuildWholePark(bool clearPark);	
	bool						RebuildFloor(bool firstTestMemory = false);
	void						RebuildNodeArray();
	void						RebuildInnerShell(GridDims newNearBounds);
	
	/* ======== Abstract metapiece functions ======== */
	
	CAbstractMetaPiece *		GetAbstractMeta(uint32 type, int *pRetIndex = NULL);
	CAbstractMetaPiece *		GetAbstractMetaByIndex(int index);
	
	/* ======== Concrete metapiece functions ======== */
	
	enum EDestroyFlags
	{
		mDONT_DESTROY_PIECES_ABOVE		= 0,
		mDESTROY_PIECES_ABOVE			= (1<<0),
		mEXCLUDE_RISERS					= (1<<1),
		mEXCLUDE_PIECES_MARKED_AS_SLID	= (1<<2),
	};
	
	CConcreteMetaPiece *		CreateConcreteMeta(CAbstractMetaPiece *pSource, bool isSoft = false);
	void						DestroyConcreteMeta(CConcreteMetaPiece *pMeta, EDestroyFlags flags);
	uint32						DestroyMetasInArea(GridDims area, EDestroyFlags flags);
	bool						DestroyMetasInArea(GridDims area, bool doNotDestroyRestartsOrFlags=false, bool onlyIfWithinArea=false);
	void						HighlightMetasInArea(GridDims area, bool on);
	//GridDims					FindBoundingArea(GridDims area);
	void						AddMetaPieceToPark(GridDims pos, CConcreteMetaPiece *pPiece);
	CConcreteMetaPiece *		RelocateMetaPiece(CConcreteMetaPiece *pMeta, int changeX, int changeY, int changeZ);
	CMapListNode *				GetMetaPiecesAt(GridDims dims, Mth::ERot90 rot = Mth::ROT_0, CAbstractMetaPiece *pPiece = NULL, bool excludeRisers = false);
	CMapListNode *				GetAreaMetaPieces(GridDims dims);
	CMapListNode *				GetMapListNode(int x, int z);
	CMapListNode *				GetConcreteMetaList() {return mp_concrete_metapiece_list;}
	int 						GetFloorHeight(int x, int z);
	
	
	bool 						AreMetasOutsideBounds(GridDims new_dims);
	bool 						EnoughMemoryToResize(GridDims new_dims);

	int 						CountGeneralMetas() {return m_num_general_concrete_metapieces;}
	int							GetDMAPieceCount() {return m_dma_piece_count;}
	
	
	/* ======== Floor height functions ======== */
	
	int							GetFloorHeight(GridDims dims, bool *pRetUniformHeight = NULL);
	#if 0
	void						SetFloorHeight(GridDims area, int newHeight, bool justDropHighest);
	#endif
	void						ResetFloorHeights(GridDims area);
	
	bool						SlideColumn(GridDims area, int bottom, int top, bool up, bool uneven);
	void						UndoSlideColumn();
	bool						ChangeFloorHeight(GridDims dims, int dir);

	/* ======== Coordinate info functions ======== */
	
	const GridDims &			GetParkNearBounds() {return m_park_near_bounds;}
	const GridDims &			GetParkFarBounds() {return m_park_far_bounds;}
	Mth::Vector					GridCoordinatesToWorld(const GridDims &cellDims, Mth::Vector *pRetDims = NULL);

	static CParkManager *		sInstance();
	
	/* ======== Piece set (group, category) functions ======== */
	
	class CPieceEntry
	{
	public:
		uint32									mNameCrc;
		const char *							mpName;
	};
	
	class CPieceSet
	{
	public:
		enum 
		{
			MAX_ENTRIES =						40,
		};
		
		uint32									mNameCrc;
		const char *							mpName;
		CPieceEntry 							mEntryTab[MAX_ENTRIES];
		int										mTotalEntries;
		int										mSelectedEntry;
		char									mVisible;
		char									mIsClipboardSet;
	};

	enum
	{
		MAX_SETS =								40,
	};

	CPieceSet &					GetPieceSet(int setNumber, int *pMenuSetNumber = NULL);
	int							GetTotalNumPieceSets() {return m_total_sets;}

	/* ======== Road mask functions ======== */
	
	bool						IsOccupiedByRoad(GridDims area);
	bool						CanPlacePiecesIn(GridDims cellDims, bool ignoreFloorHeight=false);


	/* ======== Restart node functions ===== */

	void						SetRestartTypeId(CParkGenerator::RestartType type, uint32 id);
	CParkGenerator::RestartType	IsRestartPiece(uint32 id);

	/* ======== Gap functions ===== */
	
	CGapManager::GapDescriptor *GetGapDescriptor(GridDims &area, int *pHalf);
	bool 						AddGap(CGapManager::GapDescriptor &descriptor);
	void 						RemoveGap(CGapManager::GapDescriptor &descriptor);
	void						RemoveGap(CConcreteMetaPiece *pMeta);
	void						HighlightMetasWithGaps(bool highlightOn, CGapManager::GapDescriptor *pDesc);

	//void						FreeUpMemoryForPlayingPark();
	
	void						GetSummaryInfoFromBuffer(uint8 *p_buffer, int *p_numGaps, int *p_numMetas, uint16 *p_theme, uint32 *p_todScript, int *p_width, int *p_length);
	
	#ifdef __PLAT_NGC__
	void						SwapMapBufferEndianness();
	#endif
	
protected:

	/* ======== Key members ======== */
	
	CParkGenerator *			mp_generator;
	bool						m_state_on;
	bool						m_park_is_valid;

	/* ======== Riser/floor related functions ======== */
	
	enum EFloorFlags
	{
		NONE =					0,
		mHAS_BOTTOM =			(1<<0),
		mHAS_TOP =				(1<<1),
		mNO_COVER =				(1<<2),
	};
	
	struct BuildFloorParams
	{
		/* for build_add_floor_piece() */
		int 					x, y, z;
		EFloorFlags 			flags;
		int 					heightSlotOffset;
		/* for output_riser_stack() */
		int						bottom;
		int						top;
		bool					simpleBuild;
		bool					dryRun;
		int						addCount; 	// counts metapieces
		int						addCount2;	// counts contained pieces
	};

	void						output_riser_stack(BuildFloorParams &params);
	void						build_add_floor_piece(BuildFloorParams &params);
	void						build_add_floor_piece_simple(BuildFloorParams &params);
	void						apply_meta_contained_risers_to_floor(CConcreteMetaPiece *pMetaPiece, bool add);
	
	CMapListNode *				mp_column_slide_list;
	int							m_num_metas_killed_in_slide;
	int							m_num_pieces_killed_in_slide;
		
	enum ESlideColumnFlags
	{
		mUNEVEN =				(1<<0),
		mUP	=					(1<<1),
		mFIRST_RECURSE =		(1<<2),
	};
	bool 						intersection_with_riser_containing_metas(GridDims area);
	bool						slide_column_part_one(GridDims area, int bottom, int top, ESlideColumnFlags flags, CMapListNode **ppMoveList);
	void						fix_slide_column_part_one_failure();
	void						slide_column_part_two(CMapListNode *pMoveList);
	void						kill_pieces_in_layer_under_area(GridDims area);
	void						finish_slide_column();
	bool						generate_floor_pieces_from_height_map(bool simpleBuild, bool dryRun);

	void						increment_meta_count(uint32 pieceChecksum);
	void						decrement_meta_count(uint32 pieceChecksum);
	int							get_dma_usage(uint32 pieceChecksum);

	enum
	{
		NUM_RISER_INFO_SLOTS = 	8,
	};
	
	uint32						m_riser_piece_checksum[NUM_RISER_INFO_SLOTS][3];

	#ifdef USE_BUILD_LIST
	struct SBuildListEntry
	{
		uint32 mType;
		GridDims mPos;
	};	
	int							m_build_list_size;
	SBuildListEntry				*mp_build_list_entry;
	void						create_metas_in_build_list();
	#endif
	
	/* ======== metapiece management stuff ======== */
	
	CMapListNode *				mp_concrete_metapiece_list;
	CMapListNode *				mp_abstract_metapiece_list;
	int 						m_num_concrete_metapieces;
	int							m_num_general_concrete_metapieces; // concrete metapieces not including riser, shell metapieces. Must be in park.
	int 						m_num_abstract_metapieces;
	// The DMA piece count counts pieces weighted according to their contribution to DMA usage.
	// Some pieces contribute more than 1, eg the lava piece, and any piece that has wibbling.
	// Needed so that the gauge can take into account DMA usage and not allow DMA overflow.
	int							m_dma_piece_count;
	CMapListNode * 				mp_bucket_list[FLOOR_MAX_WIDTH][FLOOR_MAX_LENGTH];

	void 						add_metapiece_to_node_list(CMetaPiece *pPieceToAdd, CMapListNode **ppList);
	void 						remove_metapiece_from_node_list(CMetaPiece *pPieceToRemove, CMapListNode **ppList);
	void						bucketify_metapiece(CConcreteMetaPiece *pPiece);
	void						debucketify_metapiece(CConcreteMetaPiece *pPiece);
		
	void						create_abstract_metapieces();
	void						destroy_concrete_metapieces(CParkGenerator::EDestroyType type);

	/* ======== Floor map/park bounds stuff ======== */
	
	int							m_theme;
	
	/*
		Within the 256X256 "super map"
		
		X and Z are positive -- must subtract 128 to get true world cell coordinates
	*/
	GridDims					m_park_near_bounds;
	GridDims					m_park_far_bounds;

	enum
	{
		TYPE_EMPTY_BLOCK		= 0,
		TYPE_FLOOR_BLOCK		= 0xaec511eb,
		TYPE_FLOOR_WALL_BLOCK	= 0x829cfc8d,
		TYPE_WALL_BLOCK			= 0x304bd5f7,
	};
	
	struct FloorHeightEntry
	{
		int						mHeight;
		/*
			Bit 0 = -16Y, 31 = 15Y
			
			Each bit set true of corresponding riser is part of metapiece at that position.
		*/
		uint32					mEnclosedFloor;
		bool					mMarkAsSlid;
		int						mSlideAmount;
	};

	FloorHeightEntry **			m_floor_height_map;

	/* ======== piece set stuff ======== */
	
	CPieceSet					m_palette_set[MAX_SETS];
	int							m_total_sets;

	/* ======== map buffer access stuff ======== */
	
	struct CompressedMapHeader
	{
		uint32					mChecksum; // CRC of every byte in file except these 4
		uint16					mSizeInBytes;
		uint16					mVersion;
		uint16					mTheme;
		uint16					mParkSize;
		uint8 					mX;
		uint8					mZ;
		uint8					mW;
		uint8					mL;
		uint16					mNumMetas;
		uint16					mNumGaps;
		uint8					mMaxPlayers;
		uint32					mTODScript;
		char					mParkName[64];
	};

	struct CompressedMapHeaderOld
	{
		uint32					mChecksum; // CRC of every byte in file except these 4
		uint16					mSizeInBytes;
		uint16					mVersion;
		uint16					mTheme;
		uint16					mParkSize;
		uint8 					mX;
		uint8					mZ;
		uint8					mW;
		uint8					mL;
		uint16					mNumMetas;
		uint16					mNumGaps;
		char					mParkName[64];
	};

	struct CompressedFloorEntry
	{
		sint8					mHeight;
	};

	// K: To allow loading of older parks that do not have gap cancel-flag info 
	struct CompressedGapOld
	{
		uint8					x[2];
		uint8					y[2];
		uint8					z[2];
		// bits 0-3 left extent, 4-7 right
		uint8					extent[2];
		// bits 0-3 first rot, 4-7 second
		uint8					rot;
		uint8					flags;
		char					text[32];
		uint16					score;
	};
	
	struct CompressedGap
	{
		uint8					x[2];
		uint8					y[2];
		uint8					z[2];
		// bits 0-3 left extent, 4-7 right
		uint8					extent[2];
		// bits 0-3 first rot, 4-7 second
		uint8					rot;
		uint8					flags;
		char					text[32];
		uint16					score;
		
		uint32					mCancelFlags;
	};
	
	enum
	{   
		VERSION					= 20004,
	};
	uint8 *						mp_compressed_map_buffer;
	
	enum ECompressedMapFlags
	{
		mNO_FLAGS				= 0,
		mIN_SYNC_WITH_PARK		= (1<<0),	// cleared if park hasn't been built yet
		mIS_VALID				= (1<<1),
		mIS_NEWER_THAN_PARK		= (1<<2),	// also set if park hasn't been built yet

		mNOT_SAVED_LOCAL		= (1<<3),
	};
	ECompressedMapFlags			m_compressed_map_flags;

	void						fake_compressed_map_buffer();
	void						write_compressed_map_buffer();
	void						read_from_compressed_map_buffer();
	void 						read_from_compressed_map_buffer_old();

	void						setup_default_dimensions();
	
	char						mp_park_name[64];
	
	/* ======== Road mask section ======== */

	enum
	{
		NUM_ROAD_PIECES			= 8,
	};
	GridDims					m_road_mask_tab[NUM_ROAD_PIECES];
	uint						m_road_mask_tab_size;

	void						setup_road_mask();

	/* ======== Restart Node section ======= */

	uint32						m_restartPieceIdTab[CParkGenerator::vNUM_RESTART_TYPES];	 // Table of checksums of the names of restart pieces


	/* ======== Lights Section ============= */
	void						setup_lighting();


	/* ======== Gap Section ============= */
	
	CGapManager	*				mp_gap_manager;
	
	static CParkManager *		sp_instance;
};




}

#endif // __SK_PARKEDITOR2_EDMAP_H

