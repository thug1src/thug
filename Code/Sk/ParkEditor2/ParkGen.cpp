#include <core/defines.h>
#include <sk/ParkEditor2/ParkGen.h>
#include <sk/ParkEditor2/EdMap.h>			// Mick:  Added as we need to iterate over the ConcreteMetaPieces
#include <core/math.h>

#include <gel/collision/collision.h>
#include <gel/collision/colltridata.h>
#include <gel/scripting/script.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <sk/scripting/nodearray.h>
#include <sk/scripting/cfuncs.h>
#include <sk/scripting/gs_file.h>
#include <sk/gamenet/gamenet.h>

#include <sys/replay/replay.h>	   // Needed to get the size of the replay buffer, for adjusting available memory

#include <gel/object/compositeobjectmanager.h>
#include <sk/components/raileditorcomponent.h>


#include <gfx/nx.h>
#include <gfx/NxSector.h>
#include <gfx/NxGeom.h>
#include <gfx/NxScene.h>
#include <sk/heap_sizes.h>

//#define	DEBUG_RESTARTS

#ifdef __PLAT_NGC__
#include <gfx/ngc/p_nxscene.h>
#endif		// __PLAT_NGC__

#ifdef __PLAT_NGPS__
#define PRINT_RAW_RESULTS 0

#if PRINT_RAW_RESULTS
#include <core/crc.h>
#include <gfx/ngps/p_nxgeom.h>
#include <gfx/ngps/nx/geomnode.h>
#include <gfx/ngps/nx/dma.h>


namespace Nx
{
	extern void find_geom_leaves( NxPs2::CGeomNode *p_node, NxPs2::CGeomNode **leaf_array, int & num_nodes);
}
#endif // PRINT_RAW_RESULTS

#endif


namespace Ed
{



const float CParkGenerator::CELL_WIDTH = 120.0f;
const float CParkGenerator::CELL_HEIGHT = 48.0f;
const float CParkGenerator::CELL_LENGTH = 120.0f;


// the first range of ids are used for regular objects and object nodes
// the second for rail nodes
const uint32 CParkGenerator::vFIRST_ID_FOR_OBJECTS 			= 2000;
const uint32 CParkGenerator::vMAX_ID_FOR_OBJECTS 			= 2000000;
const uint32 CParkGenerator::vFIRST_ID_FOR_RAILS 			= 2000001;
const uint32 CParkGenerator::vMAX_ID_FOR_RAILS 				= 4000000;
const uint32 CParkGenerator::vFIRST_ID_FOR_CREATED_RAILS	= 4000001;
const uint32 CParkGenerator::vMAX_ID_FOR_CREATED_RAILS		= 6000000;
const uint32 CParkGenerator::vFIRST_ID_FOR_RESTARTS 		= 6000001;



CPiece::CPiece()
{
	m_flags = EFlags(0);

	//mp_sector = NULL;
	m_sector_checksum = 0;
	
	mp_next_in_list = NULL;
}




CPiece::~CPiece()
{
}




void CPiece::GetCellDims(GridDims *pDims)
{
	Mth::Vector dims = GetDims();
	pDims->SetWHL((uint8) ((dims.GetX() + 2.0f) / CParkGenerator::CELL_WIDTH),
				  (uint8) ((dims.GetY() + 2.0f) / CParkGenerator::CELL_HEIGHT),
				  (uint8) ((dims.GetZ() + 2.0f) / CParkGenerator::CELL_LENGTH));
	
	/*
	if (m_sector_checksum == Script::GenerateCRC("Sk3Ed_MTra_02"))
	{
		Ryan("Sk3Ed_MTra_02 cells dims are (%d,%d,%d), world (%.4f,%.4f,%.4f)\n", 
			 pDims->GetW(), pDims->GetH(), pDims->GetL(),
			 dims.GetX(), dims.GetY(), dims.GetZ());
	}
	*/
}




CSourcePiece *CPiece::CastToCSourcePiece()
{
	Dbg_MsgAssert(this == NULL || (m_flags & mSOURCE_PIECE), ("not a source piece!"));
	return (CSourcePiece *) this;
}




CClonedPiece *CPiece::CastToCClonedPiece()
{
	Dbg_MsgAssert(this == NULL || (m_flags & mCLONED_PIECE), ("not a cloned piece!"));
	return (CClonedPiece *) this;
}




Mth::Vector	CPiece::GetDimsWithRot(Mth::ERot90 rot)
{
	CSourcePiece *p_source_piece;
	if (m_flags & mSOURCE_PIECE)
		p_source_piece = CastToCSourcePiece();
	else
		p_source_piece = CastToCClonedPiece()->GetSourcePiece();

	Dbg_Assert(p_source_piece);

	Mth::Vector result;
	if (rot & 1)
	{
		result[X] = p_source_piece->GetDims().GetZ();
		result[Z] = p_source_piece->GetDims().GetX();
	}
	else
	{
		result[X] = p_source_piece->GetDims().GetX();
		result[Z] = p_source_piece->GetDims().GetZ();
	}
	result[Y] = p_source_piece->GetDims().GetY();

	return result;
}




CSourcePiece::CSourcePiece()
{
	set_flag(CPiece::mSOURCE_PIECE);
	m_domain = mREGULAR;
	m_triggerScriptId = 0;
}




CSourcePiece::~CSourcePiece()
{
	//printf("killed CSourcePiece\n");
}




uint32 CSourcePiece::GetType()
{
	return m_sector_checksum;
}




CClonedPiece::CClonedPiece()
{
	m_id = 0;
	set_flag(CPiece::mCLONED_PIECE);
	mp_source_piece = NULL;
}




uint32 CClonedPiece::GetID()
{
	return m_id;
}




/*
	Instant function. Piece arrives at new location right away.
*/
void CClonedPiece::SetDesiredPos(const Mth::Vector & pos, ESectorEffect sectorEffect)
{
	//Dbg_MsgAssert(!(m_flags & CPiece::mIN_WORLD), ("can't change position, piece already in world"));
	m_pos = pos;

	if (sectorEffect == CHANGE_SECTOR)
	{
		#if 0
		//printf("cloned piece at (%.2f,%.2f,%.2f), dims=(%.2f,%.2f,%.2f)\n", 
		//if (mp_source_piece->GetType() == Script::GenerateCRC("Sk3Ed_Rd1b_10x10x4"))
//			printf("   cloned piece at (%.3f,%.3f,%.3f), dims=(%.3f,%.3f,%.3f)\n", 
//				   pos.GetX(), pos.GetY(), pos.GetZ(),
//				   mp_source_piece->m_dims.GetX(), mp_source_piece->m_dims.GetY(), mp_source_piece->m_dims.GetZ());
		#endif

		/*
		if (pos.GetX() == -1140.0f && pos.GetZ() == -1140.0f)
			printf("    add (%f,%f,%f), sector 0x%x\n", 
				   pos.GetX(), pos.GetY(), pos.GetZ(), m_sector_checksum);
		*/
		
		Nx::CSector *p_sector = Nx::CEngine::sGetSector(m_sector_checksum);
		Dbg_Assert(p_sector);
	
		p_sector->SetWorldPosition(pos);
	}
}




/*
	Instant function. Piece arrives at new rotation right away.
*/
void CClonedPiece::SetDesiredRot(Mth::ERot90 rot, ESectorEffect sectorEffect)
{
	//Dbg_MsgAssert(!(m_flags & CPiece::mIN_WORLD), ("can't change rotation, piece already in world"));
	m_rot = rot;

	if (sectorEffect == CHANGE_SECTOR)
	{
		Nx::CSector *p_sector = Nx::CEngine::sGetSector(m_sector_checksum);
		Dbg_Assert(p_sector);

		//Dbg_Message("Rotating piece with to %d", (int) rot * 90);
		p_sector->SetYRotation(rot);
	}
}




void CClonedPiece::SetSoftRot(float rot)
{
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(m_sector_checksum);
	Dbg_Assert(p_sector);

	Nx::CGeom *p_geom = p_sector->GetGeom();
	if (p_geom)
	{
		Mth::Matrix rot_mat;
		rot_mat.Ident();
		rot_mat.RotateY(Mth::DegToRad(rot));
		p_geom->SetOrientation(rot_mat);
	}
}




void CClonedPiece::SetScaleX(float scaleX)
{
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(m_sector_checksum);
	Dbg_Assert(p_sector);
	Mth::Vector scale_vect;
	if (m_rot & 1)
	{
		scale_vect[X] = 1.0f;
		scale_vect[Z] = scaleX;
	}
	else
	{
		scale_vect[X] = scaleX;
		scale_vect[Z] = 1.0f;
	}
	scale_vect[Y] = 1.0f;
	p_sector->SetScale(scale_vect);
}




// Turn on or off the highlight effect for the geometry that makes up a cloned piece
void CClonedPiece::Highlight(bool on, bool makePurple)
{
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(m_sector_checksum);
	if (!p_sector)
	{
		return;
	}	
	//Dbg_Assert(p_sector);

	Nx::CGeom *p_geom = p_sector->GetGeom();
	if (p_geom)
	{
		// This effect is somewhat arbitary.   I just want to get SOME effect
		if (on)
		{
			if (makePurple)
				p_geom->SetColor(Image::RGBA(160,100,160,0));
			else
				p_geom->SetColor(Image::RGBA(160,100,100,0));
			//Ryan("set color for sector=%p\n", p_sector);
		}
		else
		{
			// ClearColor flags the geom as being NOT COLORED, rather than setting the color to neutral.
			p_geom->ClearColor();		
			//Ryan("clear color for sector=%p\n", p_sector);
		}
	}
}




void CClonedPiece::SetActive(bool active)
{
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(m_sector_checksum);
	Dbg_Assert(p_sector);
	p_sector->SetActive(active);
}




void CClonedPiece::SetVisibility(bool visible)
{
	Nx::CSector *p_sector = Nx::CEngine::sGetSector(m_sector_checksum);
	Dbg_Assert(p_sector);
	uint32 mask = visible ? 0xFF : 0;
	p_sector->SetVisibility(mask);
}




Mth::Vector	CClonedPiece::GetDims()
{
	Dbg_Assert(mp_source_piece);

	Mth::Vector result;
	if (m_rot & 1)
	{
		result[X] = mp_source_piece->m_dims.GetZ();
		result[Z] = mp_source_piece->m_dims.GetX();
	}
	else
	{
		result[X] = mp_source_piece->m_dims.GetX();
		result[Z] = mp_source_piece->m_dims.GetZ();
	}
	result[Y] = mp_source_piece->m_dims.GetY();

	return result;
}




CParkGenerator::CParkGenerator()
{
	m_flags = EFlags(0);
	mp_cloned_piece_list = NULL;
	mp_source_piece_list = NULL;

	mp_cloned_scene = NULL;
	mp_source_scene = NULL;
	m_num_cloned_pieces = 0;
	m_num_source_pieces = 0;

	m_next_id = 32;

	mp_mem_region = NULL;
	mp_mem_heap = NULL;

	m_mem_usage_info.mMainHeapFree = 0;
	m_mem_usage_info.mMainHeapUsed = 0;
	m_mem_usage_info.mParkHeapFree = 0;
	m_mem_usage_info.mParkHeapUsed = 0;
	m_mem_usage_info.mLastMainUsed = 0;
	
	m_total_rail_points = 0;
	m_total_rail_linked_points = 0;
	
	m_max_players=2;
}




CParkGenerator::~CParkGenerator()
{
	// Clean up scenes (do this last)
	if (mp_cloned_scene)
	{
		delete mp_cloned_scene;
	}
	if (mp_source_scene)
	{
		delete mp_source_scene;
	}
}

int	CParkGenerator::GetResourceSize(char *name)
{
	Script::CStruct *p_resource_struct = Script::GetStructure("Ed_Resources_Info");
	int size;

	if (strcmp(name, "main_heap_base") == 0)
	{
		Script::CArray *p_values;
		p_resource_struct->GetArray(name, &p_values);
		#ifdef __PLAT_NGPS__
		size = p_values->GetInteger(0);
		#else
			#ifdef __PLAT_NGC__
			size = 4654800+((2 * 1024 * 1024)-900000);	//p_values->GetInteger(1);
			#else
			size = p_values->GetInteger(2);
			#endif
		#endif
	}
	else if(strcmp(name, "main_heap_pad") == 0)
	{
		Script::CArray *p_values;
		p_resource_struct->GetArray(name, &p_values);
#		if defined( __PLAT_NGPS__ )
		size = p_values->GetInteger(0);
#		elif defined( __PLAT_NGC__ )
		size = -2294188;	//p_values->GetInteger(1);
#		else
		size = p_values->GetInteger(2);
#		endif

	}
	else if (strcmp(name, "theme_pad") == 0)
	{
		int theme=CParkManager::sInstance()->GetTheme();
		#ifdef __PLAT_NGC__
		int theme_size[5] =
		{
			0,
			179296,
			822784,
			482944,
			444544
		};
		size = theme_size[theme];
		#else
			#ifdef __PLAT_XBOX__
				int theme_size[5] =
				{
					0,
					253728,
					564688,
					161344,
					441856
				};
				size = theme_size[theme];
			#else		
				Script::CArray *p_values;
				p_resource_struct->GetArray(name, &p_values);
				size = p_values->GetInteger(theme);
			#endif
		#endif		// __PLAT_NGC__
	}
	#ifdef __PLAT_NGC__
	else if(strcmp(name, "floor_piece_size_main") == 0)
	{
		size = 1000;
	}
	#endif
	else
	{
		if (!p_resource_struct->GetInteger(name, &size))
		{
			Dbg_MsgAssert(0, ("couldn't get resource size '%s'", name));
			return 0;
		}
	}

	return size;
}




Mem::Heap *CParkGenerator::GetParkEditorHeap()
{
	Dbg_MsgAssert(mp_mem_heap, ("no park editor heap"));
	return mp_mem_heap;
}



void CParkGenerator::SetMaxPlayers(int maxPlayers)
{
	Dbg_MsgAssert(maxPlayers>1 && maxPlayers<=GameNet::vMAX_PLAYERS,("Bad maxPlayers of %d",maxPlayers));
	m_max_players=maxPlayers;
}	

CParkGenerator::MemUsageInfo CParkGenerator::GetResourceUsageInfo(bool printInfo)
{
	Dbg_Assert(mp_mem_heap);
	Dbg_Assert(this);
	
	//int base_park_heap = 0;
	//int max_base_park_heap = 0;
	
	//int padding_size = GetResourceSize("park_heap_pad");
	//int used_park_heap = mp_mem_heap->mUsedMem.m_count;
	//int free_park_heap = mp_mem_heap->mFreeMem.m_count + mp_mem_region->MemAvailable();


	// allowed to be used, that is
	//int allowed_park_heap_mem = free_park_heap + used_park_heap - padding_size;
	//float park_heap_pct = (float) (used_park_heap - base_park_heap) / (float) (allowed_park_heap_mem - max_base_park_heap);
	//float actual_park_heap_pct = (float) (used_park_heap) / (float) (allowed_park_heap_mem);
	/*
	if (largest_free_block < padding_size)
	{
		// there is no guarantee we can safetly fit the next piece
		// placed, so make the memory meter full
		park_heap_pct = actual_park_heap_pct = 1.0001f;
	}
	*/

	Mem::Heap *p_main_heap = Mem::Manager::sHandle().BottomUpHeap();
	Mem::Region *p_main_region = p_main_heap->mp_region;
	Dbg_Assert(p_main_region);
	if (m_mem_usage_info.mMainHeapUsed != p_main_heap->mUsedMem.m_count)
	{
		m_mem_usage_info.mLastMainUsed = m_mem_usage_info.mMainHeapUsed;
	}
	
	// Mick, the replay buffer is allocated in the park editor
	// but we want to ignore it in our calculations
	// so we just subtract it from the main_heap_pad																			 
#ifdef __PLAT_NGC__
	int main_padding_size = GetResourceSize("main_heap_pad");
#else
#if __USE_REPLAYS__
	int main_padding_size = GetResourceSize("main_heap_pad") - Replay::GetBufferSize();
#else	
	int main_padding_size = GetResourceSize("main_heap_pad");
#endif
#endif
	
	main_padding_size+=(m_max_players-1)*(SKATER_HEAP_SIZE+SKATER_GEOM_HEAP_SIZE);
	
	int total_mem_available=p_main_heap->mFreeMem.m_count + p_main_region->MemAvailable();
	if (main_padding_size > total_mem_available)
	{
		//Dbg_MsgAssert(0,("main_padding_size > total_mem_available !! (%d > %d)",main_padding_size,total_mem_available));
		#ifdef	__NOPT_ASSERT__
		//printf("main_padding_size > total_mem_available !! (%d > %d)\n",main_padding_size,total_mem_available);
		#endif
		//ParkEd("main_padding_size > total_mem_available !! (%d > %d)\n",main_padding_size,total_mem_available);
	}
		
	m_mem_usage_info.mMainHeapUsed = p_main_heap->mUsedMem.m_count;
	m_mem_usage_info.mMainHeapFree = p_main_heap->mFreeMem.m_count + p_main_region->MemAvailable() - main_padding_size;
	
	
	//printf("p_main_heap->mFreeMem.m_count=%d\np_main_region->MemAvailable()=%d\nmain_padding_size=%d\n",p_main_heap->mFreeMem.m_count,p_main_region->MemAvailable(),main_padding_size);
	int park_padding_size = GetResourceSize("park_heap_pad");
	m_mem_usage_info.mParkHeapUsed = mp_mem_heap->mUsedMem.m_count;
	m_mem_usage_info.mParkHeapFree = mp_mem_heap->mFreeMem.m_count + mp_mem_region->MemAvailable() - park_padding_size;

	if (p_main_heap->LargestFreeBlock() < m_mem_usage_info.mMainHeapFree && 	// heap definitely fragmented
		p_main_heap->LargestFreeBlock() < main_padding_size && 					// no block big enough for single piece
		m_mem_usage_info.mMainHeapFree >= main_padding_size) 					// there is enough memory for single piece
	{
		// XXX
		Ryan("===================== DEFRAG ===========================\n");
		Ryan("largest: %d, free: %d, padding %d\n", 
			 p_main_heap->LargestFreeBlock(), m_mem_usage_info.mMainHeapFree, main_padding_size);
		Ryan("========================================================\n");
		m_mem_usage_info.mIsFragmented = true;
		int false_free_amt = m_mem_usage_info.mMainHeapFree - p_main_heap->LargestFreeBlock();
		m_mem_usage_info.mMainHeapFree -= false_free_amt;
		m_mem_usage_info.mMainHeapUsed += false_free_amt;
	}
	else
		m_mem_usage_info.mIsFragmented = false;
		
		
	#if 0
	if ( p_main_heap->mFreeMem.m_count > 200000)
	{
		m_mem_usage_info.mIsFragmented = true;	
	}
	#endif

	m_mem_usage_info.mTotalClonedPieces = m_num_cloned_pieces;
	m_mem_usage_info.mTotalRailPoints = m_total_rail_points;
	m_mem_usage_info.mTotalLinkedRailPoints = m_total_rail_linked_points;


	// subtract theme specific padding
	if (CParkManager::sInstance()->GetTheme() > 0)
	{
		m_mem_usage_info.mMainHeapFree -= GetResourceSize("theme_pad");
	}

	
	if (printInfo)
	{
		Ryan("=====================================================\nParkGen Usage Stats:\n\n");
		Ryan("largest free block: %d\n", mp_mem_heap->LargestFreeBlock());
		Ryan("used park heap: %d\n", m_mem_usage_info.mParkHeapUsed);
		Ryan("free park heap: %d\n", m_mem_usage_info.mParkHeapFree);
		Ryan("main heap used: %d\n", m_mem_usage_info.mMainHeapUsed);
		Ryan("main heap available: %d\n", m_mem_usage_info.mMainHeapFree);
	}
	
	return m_mem_usage_info;
	
}

int	CParkGenerator::GetMaxPlayersPossible()
{
	CParkGenerator::MemUsageInfo usage_info = GetResourceUsageInfo();
	// GetResourceUsageInfo has already subtracted the memory used by the currently set max players,
	// so add that back in so that the max max can be calculated.
	usage_info.mMainHeapFree += (m_max_players-1)*(SKATER_HEAP_SIZE+SKATER_GEOM_HEAP_SIZE);
	
	// The +1 is because at least 1 skater is assumed.
	return 	usage_info.mMainHeapFree/(SKATER_HEAP_SIZE+SKATER_GEOM_HEAP_SIZE)+1;
}



/*
	Given checksum of piece name, returns pointer to source piece.
*/
CSourcePiece *CParkGenerator::GetMasterPiece(uint32 pieceType, bool assert)
{
	CPiece *p_piece = mp_source_piece_list;
	while(p_piece)
	{
		if (p_piece->CastToCSourcePiece()->m_sector_checksum == pieceType)
			return p_piece->CastToCSourcePiece();
		p_piece = p_piece->mp_next_in_list;
	}
	
	if (assert)
	{
		Dbg_MsgAssert(0, ("couldn't not find master piece %s", Script::FindChecksumName(pieceType)));
	}
	else
		printf("couldn't not find master piece %s\n", Script::FindChecksumName(pieceType));

	return NULL;
}




CSourcePiece *CParkGenerator::GetNextMasterPiece(CSourcePiece *pLast)
{	
	if (!pLast)
		return mp_source_piece_list->CastToCSourcePiece();
	else
		return pLast->mp_next_in_list->CastToCSourcePiece();
}




/*
	Makes a copy of a source piece. The copy can then be added to a metapiece, or to the world.

	A soft piece can be smoothly rotated, like a piece used in a cursor.
*/
CClonedPiece *CParkGenerator::ClonePiece(CPiece *pMasterPiece, CPiece::EFlags flags)
{
	// make sure flags make sense
	flags = CPiece::EFlags(flags & (CPiece::mSOFT_PIECE | CPiece::mFLOOR | CPiece::mNO_RAILS));
	
	Dbg_Assert(pMasterPiece);

	CSourcePiece *pSource;
	if (pMasterPiece->m_flags & CPiece::mSOURCE_PIECE)
		pSource = pMasterPiece->CastToCSourcePiece();
	else
		pSource = pMasterPiece->CastToCClonedPiece()->mp_source_piece;

	Dbg_Assert(pSource);
	Dbg_Assert(mp_source_scene);
	Dbg_Assert(mp_cloned_scene);

	// putting cloned sectors on main heap seems best right now
	//Mem::Manager::sHandle().PushContext(mp_mem_heap);
	uint32 new_sector_checksum;
	if (flags & CPiece::mSOFT_PIECE)
	{
		// create fresh soft piece (hmm, that sounds kind of disgusting)
		new_sector_checksum = mp_source_scene->CloneSector(pSource->m_sector_checksum, mp_cloned_scene, true, false); // no instance for now (should be instance)
		//Dbg_Message("************************* New soft checksum %x", new_sector_checksum);
	}
	else
	{
		// create fresh hard piece (no, this sounds worse!)
		new_sector_checksum = mp_source_scene->CloneSector(pSource->m_sector_checksum, mp_cloned_scene, false, true); // no instance
		
		if (!(flags & CPiece::mNO_RAILS))
		{
			m_total_rail_points += pSource->m_num_rail_points;
			m_total_rail_linked_points += pSource->m_num_linked_rail_points;
		}
	}
	//Mem::Manager::sHandle().PopContext();

	// Create cloned piece, will soon live in world or as animated piece
	Mem::Manager::sHandle().PushContext(mp_mem_heap);
	CClonedPiece *p_cloned_piece = new CClonedPiece();
	Mem::Manager::sHandle().PopContext();
	p_cloned_piece->m_sector_checksum = new_sector_checksum;
	p_cloned_piece->m_id = m_next_id++;
	p_cloned_piece->mp_source_piece = pSource;
	p_cloned_piece->SetDesiredRot(Mth::ERot90(0), CClonedPiece::MARKER_ONLY);
	
	m_num_cloned_pieces++;

	p_cloned_piece->set_flag(flags);

	set_flag(mSECTORS_GENERATED);
	
	return p_cloned_piece;
}




/*
	Must be called to finalized cloned piece's presence in world.
*/
void CParkGenerator::AddClonedPieceToWorld(CClonedPiece *pPiece)
{
	Dbg_MsgAssert(!(pPiece->GetFlags() & CPiece::mIN_WORLD), ("piece already in world"));
	pPiece->set_flag(CPiece::mIN_WORLD);

	pPiece->mp_next_in_list = mp_cloned_piece_list;
	mp_cloned_piece_list = pPiece;
	
	set_flag(mPIECES_IN_WORLD);
}




void CParkGenerator::DestroyClonedPiece(CClonedPiece *pPiece)
{
	destroy_piece_impl(pPiece, DESTROY_PIECES_AND_SECTORS);
}


#ifdef __PLAT_NGPS__
#if PRINT_RAW_RESULTS
void print_ps2_data(NxPs2::CGeomNode *p_node)
{
	uint8 *p_dma = p_node->GetDma();

	int num_verts = NxPs2::dma::GetNumVertices(p_dma);
	if( num_verts >= 3 )
	{
		bool short_xyz = (NxPs2::dma::GetBitLengthXYZ(p_dma) == 16);
		Mth::Vector mesh_center = p_node->GetBoundingSphere();

		// Get DMA arrays
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
		sint32 *p_vert_array = new sint32[4 * num_verts];
		sint32 *p_uv_array = new sint32[2 * num_verts];
		uint32 *p_rgb_array = new uint32[num_verts];
		Mem::Manager::sHandle().PopContext();

		// Copy DMA data
		NxPs2::dma::ExtractXYZs(p_dma, (uint8 *) p_vert_array);
		NxPs2::dma::ExtractSTs(p_dma, (uint8 *) p_uv_array);
		NxPs2::dma::ExtractRGBAs(p_dma, (uint8 *) p_rgb_array);

		Mth::Vector v0;
		float u, v;
		Image::RGBA col;

		Dbg_Message("Number of verts: %d", num_verts);

		for (int idx = 0; idx < num_verts; idx++)
		{
			if (short_xyz)
			{
				NxPs2::dma::ConvertXYZToFloat(v0, &(p_vert_array[idx * 4]), mesh_center);
			} else {
				NxPs2::dma::ConvertXYZToFloat(v0, &(p_vert_array[idx * 4]));
			}
			NxPs2::dma::ConvertSTToFloat(u, v, &(p_uv_array[idx * 2]));
			col = *((Image::RGBA *) &(p_rgb_array[idx]));

			Dbg_Message("Vert %d", idx);
			Dbg_Message("Raw Pos (%x, %x, %x, %x)", p_vert_array[idx * 4 + 0], p_vert_array[idx * 4 + 1], p_vert_array[idx * 4 + 2], p_vert_array[idx * 4 + 3]);
			Dbg_Message("Pos (%f, %f, %f, %f)", v0[X], v0[Y], v0[Z], v0[W]);
			Dbg_Message("UV (%f, %f)", u, v);
			Dbg_Message("Color (%d, %d, %d, %d)", col.r, col.g, col.b, col.a);

		}

		// Free DMA buffers
		delete [] p_vert_array;
		delete [] p_uv_array;
		delete [] p_rgb_array;
	}
}
#endif // PRINT_RAW_RESULTS
#endif


/*
	Creates the set of source pieces. The data will have been loaded by this point.
*/
void CParkGenerator::InitializeMasterPieces(int parkW, int parkH, int parkL, int theme)
{
	if (flag_on(mMASTER_STUFF_LOADED))
		UnloadMasterPieces();
	
	set_flag(mMASTER_STUFF_LOADED);

	// When we reload the master piece, we reset the id of the generate pieces
	// so that net games will be syncronized
	m_next_id = 32;

	// set up park editor heap
	Dbg_MsgAssert(!mp_mem_region && !mp_mem_heap,(	"park editor heap already exists"));
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	#ifdef __PLAT_NGC__
	// Added 200K to the TOP_DOWN_REQUIRED_MARGIN for safety, and cancelled that by removing
	// 200K from the park editor heap. (Normally 900K)
	mp_mem_region = new Mem::AllocRegion(700000);	
	#else
	mp_mem_region = new Mem::AllocRegion(GetResourceSize("heap"));	
	#endif
	Mem::Manager::sHandle().PopContext();
	Ryan("  allocated park editor region at %p\n", mp_mem_region);
	mp_mem_heap = Mem::Manager::sHandle().CreateHeap( mp_mem_region, Mem::Allocator::vBOTTOM_UP, "Park Editor" );

	// fetch scene
	const char *p_scene_name="sk5ed";
		
	mp_source_scene = Nx::CEngine::sGetScene(p_scene_name);
	
	// set up all the source pieces
	Dbg_MsgAssert(mp_source_scene, ("couldn't get scene %s", p_scene_name));
	Lst::HashTable<Nx::CSector> *p_sector_list = mp_source_scene->GetSectorList();
	Nx::CSector *p_sector;

//	printf("Setting up park editor source pieces:\n");
	p_sector_list->IterateStart();
	while ((p_sector = p_sector_list->IterateNext()))
	{
		Mem::Manager::sHandle().PushContext(mp_mem_heap);
		CSourcePiece *p_new_piece = new CSourcePiece();
		Mem::Manager::sHandle().PopContext();
		p_new_piece->mp_next_in_list = mp_source_piece_list;
		mp_source_piece_list = p_new_piece;

		//printf ("Piece with checksum 0x%x\n",p_sector->GetChecksum());
#ifdef __PLAT_NGPS__
#if PRINT_RAW_RESULTS
		if (p_sector->GetChecksum() == Crc::GenerateCRCFromString("Sk3Ed_F_10x10"))
		{
			Dbg_Message("******* Found the target piece");
			Mth::CBBox piece_bbox(p_sector->GetBoundingBox());
			Dbg_Message("Bounding Box (%.8f, %.8f, %.8f) - (%.8f, %.8f, %.8f)",
						piece_bbox.GetMin()[X], piece_bbox.GetMin()[Y], piece_bbox.GetMin()[Z],
						piece_bbox.GetMax()[X], piece_bbox.GetMax()[Y], piece_bbox.GetMax()[Z]);

			Nx::CPs2Geom *p_geom = static_cast<Nx::CPs2Geom *>(p_sector->GetGeom());
			NxPs2::CGeomNode *p_geom_node = p_geom->GetEngineObject();
			Dbg_Assert(p_geom_node);

			// Put all the meshes in an array
			int num_meshes = 0;
			NxPs2::CGeomNode *mesh_array[10];
			Nx::find_geom_leaves(p_geom_node, mesh_array, num_meshes);
			Dbg_Assert(num_meshes == 1);
			p_geom_node = mesh_array[0];

			print_ps2_data(p_geom_node);
		}
#endif
#endif
		p_new_piece->m_sector_checksum = p_sector->GetChecksum();

		// Init dimensions
		Mth::CBBox piece_bbox(p_sector->GetBoundingBox());
		
		int cell_w = (int) ((piece_bbox.GetMax()[X] - piece_bbox.GetMin()[X] + 2.0f) / CELL_WIDTH);
		if (cell_w < 1) cell_w = 1;
		p_new_piece->m_dims[X] = cell_w * CELL_WIDTH;
		
		int cell_l = (int) ((piece_bbox.GetMax()[Z] - piece_bbox.GetMin()[Z] + 2.0f) / CELL_LENGTH);
		if (cell_l < 1) cell_l = 1;
		p_new_piece->m_dims[Z] = cell_l * CELL_LENGTH;
		
		p_new_piece->m_dims[Y] = (piece_bbox.GetMax()[Y] - piece_bbox.GetMin()[Y]);

		// piece cannot have any dimension less the that of a park editor cell
		if (p_new_piece->m_dims[Y] < CParkGenerator::CELL_HEIGHT && p_new_piece->m_dims[Y] > 0.0f)
			p_new_piece->m_dims[Y] = CParkGenerator::CELL_HEIGHT;
		
		m_num_source_pieces++;
		
		#if 0
		//printf("   %s\n      [%f,%f,%f]-[%f,%f,%f]\n", Script::FindChecksumName(p_new_piece->m_sector_checksum),
		//	   piece_bbox.GetMin()[X], piece_bbox.GetMin()[Y], piece_bbox.GetMin()[Z],
		//	   piece_bbox.GetMax()[X], piece_bbox.GetMax()[Y], piece_bbox.GetMax()[Z]);
		printf("      %s [%.2f,%.2f,%.2f]\n", Script::FindChecksumName(p_new_piece->m_sector_checksum),
			   p_new_piece->m_dims[X], p_new_piece->m_dims[Y], p_new_piece->m_dims[Z]);
		#endif
	}

	mp_cloned_scene = Nx::CEngine::sCreateScene("cloned", mp_source_scene->GetTexDict(), true);

#ifdef __PLAT_NGC__
	// Need to copy scene data here, so stuff added to the cloned scene has geometry pools.
	Nx::CNgcScene *p_Ngc_scene = static_cast<Nx::CNgcScene*>( mp_cloned_scene );
	Nx::CNgcScene *p_source_scene = static_cast<Nx::CNgcScene*>( mp_source_scene );
	NxNgc::sScene *p_scene = p_Ngc_scene->GetEngineScene();
	memcpy( p_scene, p_source_scene->GetEngineScene(), sizeof( NxNgc::sScene ) );
	p_scene->m_num_meshes			= 0;		// No meshes as yet.
	p_scene->m_num_filled_meshes	= 0;
	p_scene->mpp_mesh_list			= NULL;

	p_scene->mp_hierarchyObjects	= NULL;
	p_scene->m_numHierarchyObjects	= 0;

	p_scene->mp_hierarchyObjects	= NULL;
	p_scene->m_numHierarchyObjects	= 0;

	// Make it renderable, and make sure it doesn't try to double-delete the scene data.
	p_scene->m_is_dictionary		= false;
	p_scene->m_flags				= SCENE_FLAG_CLONED_GEOM;
#endif		// __PLAT_NGC__

	Mth::CBBox b_box;
	Mth::Vector vmin(-((float) parkW * CELL_WIDTH) / 2.0f, -((float) parkH * CELL_HEIGHT) / 2.0f, -((float) parkL * CELL_LENGTH) / 2.0f);
	b_box.AddPoint(vmin);
	Mth::Vector vmax(((float) parkW * CELL_WIDTH) / 2.0f, ((float) parkH * CELL_HEIGHT) / 2.0f, ((float) parkL * CELL_LENGTH) / 2.0f);
	b_box.AddPoint(vmax);
	mp_cloned_scene->CreateCollision(b_box);

	KillRestarts();
}

/*
// K: Added to allow cleanup of the park editor heap during play
void CParkGenerator::DeleteSourceAndClonedPieces()
{
	CPiece *p_piece = mp_source_piece_list;
	while (p_piece)
	{
		CPiece *p_next = p_piece->mp_next_in_list;
		delete p_piece;
		p_piece = p_next;
	}
	mp_source_piece_list = NULL;
	m_num_source_pieces=0;

	printf("After deleting CParkGenerator::mp_source_piece_list ...\n");	
	printf("Num CMapListNode's = %d\n",Ed::CMapListNode::SGetNumUsedItems());
	MemView_AnalyzeHeap(Ed::CParkManager::sInstance()->GetGenerator()->GetParkEditorHeap());
	
	p_piece = mp_cloned_piece_list;
	while(p_piece)
	{
		CPiece *p_next = p_piece->mp_next_in_list;
		delete p_piece;
		p_piece = p_next;
	}
	m_num_cloned_pieces=0;
	mp_cloned_piece_list=NULL;
	
	printf("After deleting CParkGenerator::mp_cloned_piece_list ...\n");	
	printf("Num CMapListNode's = %d\n",Ed::CMapListNode::SGetNumUsedItems());
	MemView_AnalyzeHeap(Ed::CParkManager::sInstance()->GetGenerator()->GetParkEditorHeap());
}
*/

/*
	Destroys the set of source pieces.
*/
void CParkGenerator::UnloadMasterPieces()
{
	if (flag_on(mPIECES_IN_WORLD))
		DestroyAllClonedPieces(DESTROY_PIECES_AND_SECTORS);

	// GARRETT: not sure if you need to do anything here
	
	CPiece *p_piece = mp_source_piece_list;
	while (p_piece)
	{
		CPiece *p_next = p_piece->mp_next_in_list;
		delete p_piece;
		p_piece = p_next;
	}
	mp_source_piece_list = NULL;
	m_num_source_pieces--;
	//m_num_source_pieces=0;
	
	clear_flag(mMASTER_STUFF_LOADED);
	
	Mem::Manager::sHandle().RemoveHeap(mp_mem_heap);
	delete mp_mem_region;
	mp_mem_heap = NULL;
	mp_mem_region = NULL;
}

/*
void CParkGenerator::DeleteParkEditorHeap()
{
	if (mp_mem_heap)
	{
		Mem::Manager::sHandle().RemoveHeap(mp_mem_heap);
		delete mp_mem_region;
		mp_mem_heap = NULL;
		mp_mem_region = NULL;
	}	
}
*/

/* 
	Should be called after adding/removing sectors
*/
void CParkGenerator::PostGenerate()
{
	Dbg_MsgAssert(flag_on(mPIECES_IN_WORLD) && flag_on(mSECTORS_GENERATED), ("no pieces or sectors available"));
}




/*
	Should be called after adding a group of cloned pieces to world.
*/
void CParkGenerator::GenerateCollisionInfo(bool assert)
{
	// if this flag IS on, then we need to call UpdateSuperSectors() to free the memory
	if (flag_on(mSECTOR_MEMORY_NEEDS_FREE))
	{
	}
	else if (!assert)
	{
		if (!flag_on(mPIECES_IN_WORLD) || !flag_on(mSECTORS_GENERATED))
			return;
	}
	else
		Dbg_MsgAssert(flag_on(mPIECES_IN_WORLD) && flag_on(mSECTORS_GENERATED), ("no pieces or sectors available"));
	
	// GARRETT
	Dbg_Assert(mp_cloned_scene);
	mp_cloned_scene->UpdateSuperSectors();
	//Dbg_Message("Adding collision to scene %x", mp_cloned_scene->GetID());
	
	// XXX
	//printf("there are %d source pieces, %d cloned pieces\n", m_num_source_pieces, m_num_cloned_pieces);
	
	set_flag(mCOLLISION_INFO_GENERATED);
	clear_flag(mSECTOR_MEMORY_NEEDS_FREE);
}


void CParkGenerator::RemoveOuterShellPieces(int theme)
{
	uint32 shell_id=CParkManager::Instance()->GetShellSceneID();
	
	Nx::CScene *p_scene = Nx::CEngine::sGetScene(shell_id);
	if (p_scene)
	{
		// Turn everything on simply by using a huge bounding box
		Mth::CBBox a(Mth::Vector(-200000,-200000,-200000),Mth::Vector(200000,200000,200000));
		p_scene->SetActiveInBox(a,true,0.0f);
		
																	 
		// Calculate the bounding box of the park up to the edges 
 		GridDims  fence;
		fence = CParkManager::Instance()->GetParkNearBounds();		 					// returns the rectangle enclosed by the fence
		Mth::Vector start = CParkManager::Instance()->GridCoordinatesToWorld(fence);		// top left cornder
		
		fence.SetXYZ(fence.GetX()+fence.GetW()-1,fence.GetY(),fence.GetZ()+fence.GetL()-1);
		Mth::Vector end = CParkManager::Instance()->GridCoordinatesToWorld(fence);		// bot right corner, in one square

		// offset into the park by half a piece		
		start[X] += CParkGenerator::CELL_WIDTH/2;
		start[Z] += CParkGenerator::CELL_LENGTH/2;
		end[X] += CParkGenerator::CELL_WIDTH/2;
		end[Z] += CParkGenerator::CELL_LENGTH/2;
		
		// The Y of the world box calculated above is not correct.... so just make it bigh enough to encompass everything
		start[Y] = -100000;
		end[Y]   =  100000;
		Mth::CBBox b(start,end);
		// Note: the above bounding box is based on the centers of cells
		// so it misses a half cell boundry around the inside of the fence
		// however, this is actually close to what I want than the true bounding box
		// as I want to provide a little slack so we don't accidentally kill 
		// things that just come inside the fence by an inch or two.
		// (remember the shell collision is not active) 
		
		
		// and turn off everything that intersects the bounding box we just calculated
		p_scene->SetActiveInBox(b,false,0.0f);
		
	}
	else
	{
		Dbg_MsgAssert(0, ("Where is my scene?! %s", Script::FindChecksumName(shell_id)));
	}
}

void CParkGenerator::CleanUpOutRailSet()
{
	m_out_rail_set.Destroy();
	m_out_rail_set.FreeAllocators();
}

// K: Factored this code out of GenerateNodeInfo because it is required by FindNearestRailPoint
void CParkGenerator::GenerateOutRailSet(CMapListNode * p_concrete_metapiece_list)
{
	CleanUpOutRailSet();
	
	Mdl::Skate * p_skate_mod =  Mdl::Skate::Instance();
	if (p_skate_mod->m_cur_level == CRCD(0xe8b4b836,"load_sk5ed"))
	{
		// K: Not using the top down heap whilst in the editor to avoid running out of memory, 
		// since this causes a 192K spike in memory usage when doing a test play.
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
	}
	else	
	{
		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	}
	m_out_rail_set.SetupAllocators(GetResourceSize("out_railpoint_pool"), GetResourceSize("out_railstring_pool"), false);
	Mem::Manager::sHandle().PopContext();

	// Need to iterate over the pieces here, meaning we'd iterate over the meta pieces
	// and then find the pieces they are composed of.....
	// should be similar to rebuilding the park (in terms of iterating over the pieces)
	
	// We have been passed in the list of concrete metapiece
	// (the pieces that have actually been placed in the level
	CMapListNode *p_node = p_concrete_metapiece_list;
	while(p_node)
	{
		CConcreteMetaPiece *p_meta = p_node->GetConcreteMeta();

		if (p_meta->IsInPark() && !p_meta->NoRails())
		{
			// Iterate over the pieces in the metapiece
			int pieces = p_meta->CountContainedPieces();
			for (int i=0;i<pieces;i++)
			{
				CPiece * p_base_piece = p_meta->GetContainedPiece(i);
				CClonedPiece *pPiece = p_base_piece->CastToCClonedPiece();	    
	
	//			if (!(pPiece->m_type & CPiece::vNO_RAILS))
				{
					// go through all rail strings associated with source object
					for (int n = 0; ;n++)
					{
						uint32 piece_checksum = pPiece->mp_source_piece->m_sector_checksum;
						RailString *pSrcString = m_in_rail_set.GetString(piece_checksum, n);
						
						if (pSrcString)
						{
//							printf("creating rail string %d for %s, meta=%s\n", 
//								   n, Script::FindChecksumName(piece_checksum), Script::FindChecksumName(p_meta->GetNameChecksum()));
							RailString *pOutString = m_out_rail_set.CreateRailString();
							uint32	trickob_id = pPiece->GetID();
							if (p_meta->IsRiser())
							{
								// objects that are JUST risers cannot be trickobs
								trickob_id = 0;
							}
							pOutString->CopyOffsetAndRot(pSrcString, pPiece->m_pos, pPiece->m_rot, trickob_id);
							m_out_rail_set.AddString(pOutString);
						}
						else
						{
							//printf ("No rails here for %x: %s\n", piece_checksum, Script::FindChecksumName(piece_checksum));
							break;
						}
					}		
				} // end if (!(pPiece->m_type & Piece::vNO_RAILS))
			}
		}
		p_node = p_node->GetNext();
	}
}

/*
	For rail generation, etc.
*/
void CParkGenerator::GenerateNodeInfo(CMapListNode * p_concrete_metapiece_list)
{
	Dbg_MsgAssert(flag_on(mPIECES_IN_WORLD) && flag_on(mSECTORS_GENERATED), ("no pieces or sectors available"));
	
	int		total_nodes = 0;
	
	
	/*
	========================================================
	Begin Rail Generation
	========================================================
	*/

	printf("Making rail objects\n");

	GenerateOutRailSet(p_concrete_metapiece_list);	
	
	total_nodes	+= m_out_rail_set.CountPoints();
	
	
	
	/*
	========================================================
	End Rail Generation
	========================================================
	*/


	#if 1	


	printf("Generating node array\n");
	
	// count up regular object nodes
	int object_nodes = 0;
	CPiece *p_piece = mp_cloned_piece_list;
	while (p_piece)
	{
		if (!(p_piece->m_flags & CPiece::mFLOOR))
			object_nodes++;
		p_piece = p_piece->mp_next_in_list;
	}
	// count up object nodes
	total_nodes += object_nodes;
	
	#endif
	

	#if 1
	/*
	========================================================
	Do Some Restart Stuff
	========================================================
	*/
	
	// count up restart nodes
	for (int res = 0; res < vNUM_RESTARTS; res++)
	{
		if (m_restart_tab[res].type != vEMPTY)
			total_nodes++;
		// one player restarts are repeated as multiplayer
		if (m_restart_tab[res].type == vONE_PLAYER)
			total_nodes++;
		// HORSE restarts are repeated as multiplayer
		if (m_restart_tab[res].type == vHORSE)
			total_nodes++;
		// flag restart nodes will generate 3 extra nodes in addition to a restart
		if (
			m_restart_tab[res].type == vRED_FLAG
			|| m_restart_tab[res].type == vGREEN_FLAG
			|| m_restart_tab[res].type == vBLUE_FLAG
			|| m_restart_tab[res].type == vYELLOW_FLAG
		)
		{
		    total_nodes += 3; 
		}
			
	}

	#endif
	
	// Add in the number of nodes needed for the created rails.	
	int num_nodes_required=Obj::GetRailEditor()->GetTotalRailNodesRequired();
	Dbg_MsgAssert((uint32)num_nodes_required <= vMAX_ID_FOR_CREATED_RAILS-vFIRST_ID_FOR_CREATED_RAILS+1,("Too many created-rail nodes"));
	total_nodes+=num_nodes_required;
	
	
	/*
	========================================================
	Actually Create Node Array
	========================================================
	*/


	SkateScript::CreateNodeArray(total_nodes, false);

	Script::CArray *pNodeArray = Script::GetArray(CRCD(0xc472ecc5,"NodeArray"));
	int current_node_num = 0;
	#if 1
	set_up_object_nodes(pNodeArray, &current_node_num);
	#endif
	
	set_up_rail_nodes(pNodeArray, &current_node_num);
	
	Obj::GetRailEditor()->SetUpRailNodes(pNodeArray, &current_node_num, vFIRST_ID_FOR_CREATED_RAILS);
	
	#if 1
	set_up_restart_nodes(pNodeArray, &current_node_num);
	#endif
	
	CleanUpOutRailSet();

	
	printf("Calling CreateNodeNameHashTable()\n");
	SkateScript::CreateNodeNameHashTable();
	printf("Calling ScriptParseNodeArray()\n");
	CFuncs::ScriptParseNodeArray(NULL, NULL);

//	m_node_array_is_set_up = true;

	
	set_flag(mNODE_INFO_GENERATED);
}




/*
	Can use 'type' parameter only to remove only CPieces, but not underlying sector. This is useful
	for freeing up memory no longer needed once park is generated.
*/
void CParkGenerator::DestroyAllClonedPieces(EDestroyType type)
{
	if (type == DESTROY_PIECES_AND_SECTORS)
	{
		Obj::GetRailEditor()->DestroyRailGeometry();
		Obj::GetRailEditor()->DestroyPostGeometry();
	}	
	
	CPiece *p_entry = mp_cloned_piece_list;
	while(p_entry)
	{
		CPiece *p_next_entry = p_entry->mp_next_in_list;
		destroy_piece_impl(p_entry->CastToCClonedPiece(), type);
		p_entry = p_next_entry;
	}
	
	if (type == DESTROY_PIECES_AND_SECTORS)
		clear_flag(mSECTORS_GENERATED);
	clear_flag(mPIECES_IN_WORLD);

	if (type == DESTROY_PIECES_AND_SECTORS)
	{
		// CHUNKY
		Dbg_Assert(mp_cloned_scene);
		mp_cloned_scene->ClearSuperSectors();
	}	
}




void	CParkGenerator::HighlightAllPieces(bool highlight)
{
	CPiece *p_entry = mp_cloned_piece_list;
	while(p_entry)
	{
		CPiece *p_next_entry = p_entry->mp_next_in_list;
		p_entry->CastToCClonedPiece()->Highlight(highlight);
		p_entry = p_next_entry;
	}
}




void	CParkGenerator::SetGapPiecesVisible(bool visible)
{
#if 1	
	// Nothing doing here yet, eventually will turn off the wireframe pieces
	CPiece *p_entry = mp_cloned_piece_list;
	while(p_entry)
	{
		CPiece *p_next_entry = p_entry->mp_next_in_list;
		if (p_entry->CastToCClonedPiece()->GetFlags() & CPiece::mGAP)
		{
			//Dbg_Assert(0);
			p_entry->CastToCClonedPiece()->SetVisibility(visible);
		}
		p_entry = p_next_entry;
	}
#endif
}




void	CParkGenerator::SetRestartPiecesVisible(bool visible)
{
	Ryan("the stars go in and the stars go out %d\n", visible);
	CPiece *p_entry = mp_cloned_piece_list;
	while(p_entry)
	{
		CPiece *p_next_entry = p_entry->mp_next_in_list;
		uint32 id = p_entry->CastToCClonedPiece()->mp_source_piece->GetType();
		if (CParkManager::Instance()->IsRestartPiece(id))
		{
			p_entry->CastToCClonedPiece()->SetActive(visible);	
		}
		p_entry = p_next_entry;
	}
}




void CParkGenerator::SetLightProps(int num_lights, 
						  float amb_const_r, float amb_const_g, float amb_const_b, 
						  float falloff_const_r, float falloff_const_g, float falloff_const_b,
						  float cursor_ambience)
{
	
	Dbg_MsgAssert(num_lights <= vMAX_LIGHTS, ("too many lights"));
	m_numLights = num_lights;
	
	m_ambientLightIntensity.r = amb_const_r;
	m_ambientLightIntensity.g = amb_const_g;
	m_ambientLightIntensity.b = amb_const_b;
	m_falloffLightIntensity.r = falloff_const_r;
	m_falloffLightIntensity.g = falloff_const_g;
	m_falloffLightIntensity.b = falloff_const_b;

	m_cursorAmbience = cursor_ambience;
}




void CParkGenerator::SetLight(Mth::Vector &light_pos, int light_num)
{
	
	Dbg_MsgAssert(light_num < m_numLights, ("out of light slots"));

	m_lightTab[light_num] = light_pos;
}

											   
// Given the id of a piece in the world, apply the lights to it											   
void CParkGenerator::CalculateLighting(CClonedPiece *p_piece)
{	
	Nx::CSector *p_sector= Nx::CEngine::sGetSector(p_piece->m_sector_checksum);
	Dbg_MsgAssert(p_sector,("Trying to Calculate lighting on non-existant sector"))
	Nx::CGeom 	*p_geom = p_sector->GetGeom();
	Dbg_MsgAssert(p_geom,("sector does not have geometry"))
	
	Nx::CSector *p_source_sector= Nx::CEngine::sGetSector(p_piece->GetSourcePiece()->GetType());
	Dbg_MsgAssert(p_source_sector,("Trying to Calculate lighting on sector with no source sector"))
	Nx::CGeom 	*p_source_geom = p_source_sector->GetGeom();
	Dbg_MsgAssert(p_source_geom,("source sector does not have geometry"))

	//
	// Do renderable geometry
	int verts = p_geom->GetNumRenderVerts();
	Dbg_MsgAssert(verts == p_source_geom->GetNumRenderVerts(),("source and clone have differing no of verts (%d, %d)\n",verts,p_source_geom->GetNumRenderVerts()));

	if (verts)
	{
		Mth::Vector	*p_verts = new Mth::Vector[verts];
		Image::RGBA	*p_colors = new Image::RGBA[verts];
		p_geom->GetRenderVerts(p_verts);
		p_source_geom->GetRenderColors(p_colors);
	
		Image::RGBA *p_color = p_colors;
		Mth::Vector *p_vert = p_verts;
		for (int i = 0; i < verts; i++)
		{
			CalculateVertexLighting(*p_vert, *p_color);

			p_color++;
			p_vert++;
		} // end for

		// Set the colors on the actual new geom, not on the source...		
		p_geom->SetRenderColors(p_colors);
		
		delete [] p_verts;
		delete [] p_colors;
	} // end if
	else
	{
		// debuggery
		//p_geom->SetColor(Image::RGBA(0,0,100,0));
	}


	//
	// Do collision geometry
	Nx::CCollObjTriData *p_coll_tri_data = p_geom->GetCollTriData();
	Nx::CCollObjTriData *p_source_coll_tri_data = p_source_geom->GetCollTriData();

	Dbg_MsgAssert(p_coll_tri_data, ("sector does not have collision"));
	Dbg_MsgAssert(p_source_coll_tri_data, ("source sector does not have collision"));

#ifdef __PLAT_NGC__
	int faces = p_coll_tri_data->GetNumFaces();
	Dbg_MsgAssert(faces == p_source_coll_tri_data->GetNumFaces(), ("source and clone have differing no of collision faces (%d, %d)\n",verts,p_source_coll_tri_data->GetNumFaces()));

	Image::RGBA color;
	unsigned char intensity;
	unsigned int avg_color;
	for (int f = 0; f < faces; f++)
	{
		for ( int c = 0; c < 3; c++ )
		{
			intensity = p_source_coll_tri_data->GetVertexIntensity(f, c);

			color.r = intensity;
			color.g = intensity;
			color.b = intensity;
			color.a = 255;

			CalculateVertexLighting(p_coll_tri_data->GetRawVertexPos(p_coll_tri_data->GetFaceVertIndex(f, c)), color);
			avg_color = (color.r + color.g + color.b) / 3;
			p_coll_tri_data->SetVertexIntensity(f, c, avg_color);
		}
	}
#else
	verts = p_coll_tri_data->GetNumVerts();
	Dbg_MsgAssert(verts == p_source_coll_tri_data->GetNumVerts(), ("source and clone have differing no of collision verts (%d, %d)\n",verts,p_source_coll_tri_data->GetNumVerts()));

	Image::RGBA color;
	unsigned char intensity;
	unsigned int avg_color;
	for (int i = 0; i < verts; i++)
	{
		intensity = p_source_coll_tri_data->GetVertexIntensity(i);

		color.r = intensity;
		color.g = intensity;
		color.b = intensity;
		color.a = 255;

		CalculateVertexLighting(p_coll_tri_data->GetRawVertexPos(i), color);

		avg_color = (color.r + color.g + color.b) / 3;
		p_coll_tri_data->SetVertexIntensity(i, avg_color);

	}
#endif // __PLAT_NGC__
}


// apply lights to a vert											   
void CParkGenerator::CalculateVertexLighting(const Mth::Vector & vert, Image::RGBA & color)
{
	LightIntensity i_amb;

	i_amb.r = (float) color.r 	* m_ambientLightIntensity.r;
	i_amb.g = (float) color.g 	* m_ambientLightIntensity.g;
	i_amb.b = (float) color.b 	* m_ambientLightIntensity.b;

	float i_spot_light = 0.0f;
	Mth::Vector *pLightTabEntry = m_lightTab;
	for (int l = 0; l < m_numLights; l++)
	{
		float dist_x = vert.GetX() - pLightTabEntry->GetX();
		//float dist_y = vert.GetY() - pLightTabEntry->GetY();
		float dist_y = 400;	// Mick:  Patched to ignore height, otherwise we have burningly bright spots on high walls
							// you can decrease this number make the lighhing more intense
							// (note, this means no verticle variation in brightness in the park editor
		float dist_z = vert.GetZ() - pLightTabEntry->GetZ();

		float dist_squared = dist_x * dist_x + dist_y * dist_y + dist_z * dist_z;
		i_spot_light += 1.0f / dist_squared;

		pLightTabEntry++;
	}
	i_spot_light *= 255.0f;

	i_amb.r += i_spot_light * m_falloffLightIntensity.r;
	if (i_amb.r > 255.0)
		color.r = 255;
	else
		color.r = (uint8) (i_amb.r);

	i_amb.g += i_spot_light * m_falloffLightIntensity.g;
	if (i_amb.g > 255.0)
		color.g = 255;
	else 
		color.g = (uint8) (i_amb.g);

	i_amb.b += i_spot_light * m_falloffLightIntensity.b;
	if (i_amb.b > 255.0)
		color.b = 255;
	else
		color.b	= (uint8) (i_amb.b);
}


/*
	See comments in DestroyAllClonedPieces() for 'destroyType'
*/
void CParkGenerator::destroy_piece_impl(CClonedPiece *pPiece, EDestroyType destroyType)
{
	// XXX
	if (pPiece->m_flags & CPiece::mGAP)
		Ryan("@@@ destroying gap piece 0x%x\n", pPiece);

	if (!(pPiece->m_flags & CClonedPiece::mSOFT_PIECE))
	{
		// remove from list	first
		CPiece *p_entry = mp_cloned_piece_list;
		CPiece *p_prev = NULL;
		while(p_entry)
		{
			if (p_entry == pPiece)
			{
				if (p_prev)
					p_prev->mp_next_in_list = p_entry->mp_next_in_list;
				else
					mp_cloned_piece_list = p_entry->mp_next_in_list;
				break;
			}
			
			p_prev = p_entry;
			p_entry = p_entry->mp_next_in_list;
		}
		Dbg_MsgAssert(p_entry, ("piece not found"));
		
		if (!(pPiece->m_flags & CPiece::mNO_RAILS))
		{
			m_total_rail_points -= pPiece->mp_source_piece->m_num_rail_points;
			m_total_rail_linked_points -= pPiece->mp_source_piece->m_num_linked_rail_points;
		}
	}

	// now actually destroy piece
	if (destroyType == DESTROY_PIECES_AND_SECTORS)
	{
		#if 0
		printf("    Gott mit uns (%f,%f,%f)\n", 
			   pPiece->GetPos().GetX(), pPiece->GetPos().GetY(), pPiece->GetPos().GetZ());
		#endif
		// destroy underlying sector
		mp_cloned_scene->DeleteSector(pPiece->m_sector_checksum);
		
		#ifdef __PLAT_NGC__
		//printf("Called DeleteSector\n");
		Nx::CEngine::sFinishRendering();
		#endif
	}
	
	m_num_cloned_pieces--;
	
	delete pPiece;

	if (!mp_cloned_piece_list)
		clear_flag(mPIECES_IN_WORLD);
	set_flag(mSECTOR_MEMORY_NEEDS_FREE);
}



// Read in the node array containing the rail information
// and parse this into the park editor format....

// called from CParkManager::Initialize()
void CParkGenerator::ReadInRailInfo()
{
	const uint32 crc_cluster 		= 0x1a3a966b;
    const uint32 crc_class 			= 0x12b4e660;
	const uint32 crc_links 			= 0x2e7d5ee7;

	printf ("Starting ReadInRailInfo()\n");

// Mick - Not sure where this should be set up
	m_in_rail_set.SetupAllocators(GetResourceSize("in_railpoint_pool"), GetResourceSize("in_railstring_pool"), true);

	
	// Ken: This function has gone now, since the new ability to unload a qb has made it redundant.
	//Script::RemoveOldTriggerScripts();
	
	// Remove any spawned scripts that might have been left over from the previous level.
	Script::DeleteSpawnedScripts();	
		
	// Load the QB
//	SkateScript::LoadQB(PIECE_SET_QB[m_currentThemeNumber]);
//	SkateScript::LoadQB("levels\\sk4ed\\sk4ed.qb");			  // assume it has already been loaded (NOTE: If not, then you will have problems with PRE files, as the .QBs do not go in the pre file)
	// Ensure it has a node array in it
	Script::CArray *pNodeArray=Script::GetArray(CRCD(0xc472ecc5,"NodeArray"));
	Dbg_MsgAssert(pNodeArray,("Park editor node array not found"));
		
	// this will load up sounds
	Script::RunScript("LoadTerrain");
	
	Ryan("\n*** Reading in rail info ***\n");
	m_in_rail_set.Destroy();
	// BAD MEM
	m_processed_node_tab = new int[256];
	m_processed_node_tab_entries = 0;
	m_temp_node_tab = new TempNodeInfo[2048];

	
	int index = 0;
	for ( ; index < (int)pNodeArray->GetSize(); index++)
	{
		Script::CScriptStructure *pStruct = pNodeArray->GetStructure(index);
	
		m_temp_node_tab[index].classCrc = 0;
		pStruct->GetChecksum(crc_class, &m_temp_node_tab[index].classCrc);
		m_temp_node_tab[index].cluster = 0;
		pStruct->GetChecksum(crc_cluster, &m_temp_node_tab[index].cluster);
		m_temp_node_tab[index].pLinkArray = NULL;
		pStruct->GetArray(crc_links, &m_temp_node_tab[index].pLinkArray);
	}

	
	const bool debug_rail_read = true;
	
	// XXX
	Ryan("number of node entries is %d\n", pNodeArray->GetSize());
	
	index = 0;
	// loop through all LevelGeometry objects in node array.
	// for each one, locate associated rail nodes (if any) and create RailString.
	while(1)
	{
		// get cluster checksum of LevelGeometry object, will be used to
		// look up rail nodes
		uint32 cluster_crc = scan_for_cluster(pNodeArray, index);
		if (index >= (int)pNodeArray->GetSize())
		{
			printf ("No more clusters, exiting\n"); 
			break;
		}
		
//		printf("source piece %s for cluster \n", Script::FindChecksumName(cluster_crc));		
		
		//debug_rail_read = (cluster_crc == 0x21997899);
#ifdef __NOPT_ASSERT__
		CPiece *pMasterPiece = GetMasterPiece(cluster_crc);
		
		CSourcePiece *pSourcePiece = pMasterPiece->CastToCSourcePiece();
		
		Dbg_MsgAssert(pSourcePiece, ("no source piece %s found", Script::FindChecksumName(cluster_crc)));		
#endif		// __NOPT_ASSERT__
		if (debug_rail_read)
			Ryan("found cluster %s at index %d\n", Script::FindChecksumName(cluster_crc), index-1);

		Mth::Vector v;
		RailPoint::RailType type;
			   
		// find all rail strings associated with current LevelGeometry object
		// m_processed_node_tab[] keeps track of strings already processed
		m_processed_node_tab_entries = 0;
		while (1)
		{
			bool string_is_loop = false;
			
			// try to locate a rail node that matches the cluster and has no links
			// if it fails to find one, it will try to return one that matches the cluster and does have links
			int last_node_in_set = scan_for_rail_node(pNodeArray, cluster_crc, -1, &v, &string_is_loop, &type);
			if (last_node_in_set == -1)
				break;

			int current_node = last_node_in_set;
			
			RailString *pCurrentString = m_in_rail_set.CreateRailString();
			pCurrentString->m_id = cluster_crc;
			pCurrentString->m_isLoop = string_is_loop;

			RailPoint *pPoint = m_in_rail_set.CreateRailPoint();
			//pPoint->m_pos.Set(v.GetX() - pSourcePiece->m_pos.GetX(), v.GetY() - pSourcePiece->m_pos.GetY(), v.GetZ() + pSourcePiece->m_pos.GetZ());
			pPoint->m_pos.Set(v.GetX(), v.GetY(), v.GetZ());
			pPoint->m_type = type;
			if (debug_rail_read)
				Ryan("* [%.1f,%.1f,%.1f]", pPoint->m_pos.GetX(), pPoint->m_pos.GetY(), pPoint->m_pos.GetZ());
			pCurrentString->AddPoint(pPoint);
			
			// go through all points in the string (after the first)
			while (1)
			{
				current_node = scan_for_rail_node(pNodeArray, cluster_crc, current_node, &v, &string_is_loop, &type);
				if (current_node == -1)
				{
					if (debug_rail_read && pCurrentString->m_isLoop)
						Ryan(" (loop)");
					break;
				}

				
				pPoint = m_in_rail_set.CreateRailPoint();
				//pPoint->m_pos.Set(v.GetX() - pSourcePiece->m_pos.GetX(), v.GetY() - pSourcePiece->m_pos.GetY(), v.GetZ() + pSourcePiece->m_pos.GetZ());
				pPoint->m_pos.Set(v.GetX(), v.GetY(), v.GetZ());
				pPoint->m_type = type;
				if (debug_rail_read)
					Ryan("-[%.1f,%.1f,%.1f]", pPoint->m_pos.GetX(), pPoint->m_pos.GetY(), pPoint->m_pos.GetZ());
				pCurrentString->AddPoint(pPoint);
			}

			if (debug_rail_read)
				Ryan("\n");
			m_in_rail_set.AddString(pCurrentString);
		} // end while
	} // end while

	delete m_processed_node_tab;
	delete m_temp_node_tab;

	scan_in_trigger_info(pNodeArray);
//
//	m_nodeArrayIsSetup = true;
//	DestroyCollisionDataAndNodeArray();

	// now that rail info is set up, it's safe to do this next block of code
	CPiece *p_piece = mp_source_piece_list;
	while(p_piece)
	{
		CSourcePiece *p_source_piece = p_piece->CastToCSourcePiece();

		p_source_piece->m_num_rail_points = 0;
		p_source_piece->m_num_linked_rail_points = 0;
		int string_num = 0;
		while(1)
		{
			RailString *p_rail_string = m_in_rail_set.GetString(p_source_piece->GetType(), string_num++);
			if (!p_rail_string)
				break;
			p_source_piece->m_num_rail_points += p_rail_string->Count();
			p_source_piece->m_num_linked_rail_points += p_rail_string->CountLinkedPoints();
		}

		p_piece = p_piece->mp_next_in_list;
	}
}




// called from CParkManager::Destroy()
void CParkGenerator::DestroyRailInfo()
{
	m_in_rail_set.Destroy();
	m_in_rail_set.FreeAllocators();
}







// returns checksum of first found cluster, increases index appropriately
// called from ReadInRailInfo()
uint32 CParkGenerator::scan_for_cluster(Script::CArray *pNodeArray, int &index)
{
    const uint32 crc_class 			= 0x12b4e660;
    const uint32 crc_levelgeometry = 0xbf4d3536;		//0xdabd3086;
	const uint32 crc_name 			= 0xa1dc81f9;

	int total_entries = pNodeArray->GetSize();

	/*
		Given a node like...
		
		{
			// Node 471
			Position = (-0.002991,1200.000000,0.000854)
			Angles = (0.000000,0.000000,0.000000)
			Name = Sk3Ed_Shell_Col_16x16_01
			Class = levelgeometry
			CreatedAtStart
		}
		
		... make sure it's level geometry, then find the value
		of 'Name'
	*/
	while (index < total_entries)
	{
		Script::CStruct *pStruct = pNodeArray->GetStructure(index);
		
		uint32 node_class = 0;
		pStruct->GetChecksum(crc_class, &node_class);
		
		if (node_class == crc_levelgeometry)
		{
			uint32 name;
			if (pStruct->GetChecksum(crc_name, &name))
			{
				index++;
				return name;
			}			
		}

		index++;
	}

	return 0;
}




/* 
	returns number of node
	3 ways of calling:
		-link set: find rail node with that link
		-cluster set, link -1: find rail node with that cluster, but no link
		-cluster set, link -2: find rail node with that cluster, can have a link
	
	Called from ReadInRailInfo()
*/
int CParkGenerator::scan_for_rail_node(Script::CArray *pNodeArray, uint32 cluster, int link, Mth::Vector *pVector, bool *pHasLinks, RailPoint::RailType *pType)
{
	
	
//	const uint32 crc_position 		= 0xb9d31b0a;
	//const uint32 crc_links 			= 0x2e7d5ee7;
	//const uint32 crc_cluster 		= 0x1a3a966b;
	const uint32 crc_terraintype	= 0x54cf8532;
//	const uint32 crc_terrain_wood	= 0x39075ea5;
	
    //const uint32 crc_class 			= 0x12b4e660;
    const uint32 crc_railnode 		= 0x8e6b02ad;

	int found_node_with_link_index = -1;
	int found_node_without_link_index = -1;
	
	/*
		A little optimization:
	
		We first call this function with link<0, so at that time
		store the index of the first node belonging to the requested
		cluster.
		
		Several following calls to this function will start the node
		search from this point
	*/
	
	static int first_node_in_cluster;
	if (link < 0)
		first_node_in_cluster = 0;

	int total_entries = pNodeArray->GetSize();
	for (int n = first_node_in_cluster; n < total_entries; n++)
	{
		//Script::CScriptStructure *pStruct = pNodeArray->GetStructure(n);
	
		uint32 node_class = m_temp_node_tab[n].classCrc;
		if (node_class == crc_railnode)
		{
			// find link
			int found_link = -1;
			Script::CArray *pLinkArray = m_temp_node_tab[n].pLinkArray;
			if (pLinkArray)
			{
				Dbg_MsgAssert(pLinkArray->GetSize() == 1, ("Rail node %d has more than one link (%d)",n,pLinkArray->GetSize()));
				found_link = pLinkArray->GetInt(0);
			}
			
			uint32 found_cluster = m_temp_node_tab[n].cluster;
			//Ryan("   cluster is %s, link is %d\n", Script::FindChecksumName(found_cluster), found_link);

			if (link >= 0)
			{
				if (found_link == link)
				{
					found_node_with_link_index = n;
					for (int i = 0; i < m_processed_node_tab_entries; i++)
					{
						if (m_processed_node_tab[i] == found_node_with_link_index)
							// we've already processed this node
							found_node_with_link_index = -1;
					}
					if (found_node_with_link_index != -1)
						// in this case, we have all the info we need, so exit loop
						break;
				}
			}
			else if (cluster == found_cluster)
			{
				if (!first_node_in_cluster)
					first_node_in_cluster = n;
				
				if (found_link == -1)
				{
					found_node_without_link_index = n;
					for (int i = 0; i < m_processed_node_tab_entries; i++)
					{
						if (m_processed_node_tab[i] == found_node_without_link_index)
							// we've already processed this node
							found_node_without_link_index = -1;
					}
					if (found_node_without_link_index != -1)
						// this is the node we're looking for, so exit loop
						break;
				}
				else if (found_node_with_link_index == -1) // && found_link >= 0
				{
					found_node_with_link_index = n;
					for (int i = 0; i < m_processed_node_tab_entries; i++)
					{
						if (m_processed_node_tab[i] == found_node_with_link_index)
							// we've already processed this node
							found_node_with_link_index = -1;
					}
					// even if not already processed, this node is secondary priority, 
					// so we will not be exiting the loop
				}
			} // end else if
		} // if (node_class == crc_railnode)
	}
		
	// the node without a link has greater priority
	int match_index = -1;
	if (found_node_with_link_index != -1)
	{
		match_index = found_node_with_link_index;
		*pHasLinks = true;
	}
	if (found_node_without_link_index != -1)
	{
		match_index = found_node_without_link_index;	
		*pHasLinks = false;
	}
	if (match_index == -1) return -1;

	// store this node so we won't use it twice
	m_processed_node_tab[m_processed_node_tab_entries++] = match_index;
	// fetch position info (will be returned to caller)
	Script::CScriptStructure *pFound = pNodeArray->GetStructure(match_index);
	
	pFound->GetVector(CRCD(0x7f261953,"Pos"), pVector, true);

// GJ:  The following was incorrect, because the Z component should have been negated,
// however, the function that was supposed to use this data should have negated it again,
// but didn't, meaning that the 2 bugs ended up cancelling each other out.
// The new-style 'pos' vector is already correct and needs no negation.
//	pFound->GetVector(CRCD(0xb9d31b0a,"Position"), pVector, false);
	
	uint32 terrain_type; // terrain_wood
	pFound->GetChecksum(crc_terraintype, &terrain_type, true);
	
//	if (terrain_type == crc_terrain_wood)
//		*pType = RailPoint::vWOOD;
//	else
//		*pType = RailPoint::vMETAL;

	*pType =  terrain_type; 

	return match_index;
}




// 	Should be called from ReadInRailInfo(), but isn't
void CParkGenerator::scan_in_trigger_info(Script::CArray *pNodeArray)
{
	//const uint32 crc_position 		= 0xb9d31b0a;
    const uint32 crc_class 			= 0x12b4e660;
    const uint32 crc_levelgeometry = 0xbf4d3536;		//0xdabd3086;
	const uint32 crc_name 			= 0xa1dc81f9;
    const uint32 crc_triggerscript 	= 0x2ca8a299;

	for (uint32 n = 0; n < pNodeArray->GetSize(); n++)
	{
		Script::CScriptStructure *pStruct = pNodeArray->GetStructure(n);
		
		uint32 node_class = 0;
		pStruct->GetChecksum(crc_class, &node_class);
		
		if (node_class == crc_levelgeometry)
		{
			uint32 piece_name = 0;
			pStruct->GetChecksum(crc_name, &piece_name);
			uint32 trigger_name = 0;
			pStruct->GetChecksum(crc_triggerscript, &trigger_name);
			
			if (piece_name && trigger_name)
			{
				// XXX
				Ryan("Setting up trigger script: piece=%s script=%s\n", Script::FindChecksumName(piece_name), Script::FindChecksumName(trigger_name));
				CPiece *pPiece = GetMasterPiece(piece_name, true);
				CSourcePiece *pSourcePiece = pPiece->CastToCSourcePiece();
				pSourcePiece->m_triggerScriptId = trigger_name;
			}
		}
	}
	Ryan("done with all rail reading\n");
}




void CParkGenerator::set_up_object_nodes(Script::CArray *pNodeArray, int *pNodeNum)
{
	

	Ryan("set_up_object_nodes: %d/%d\n", Mem::CPoolable<Script::CComponent>::SGetNumUsedItems(), Mem::CPoolable<Script::CComponent>::SGetTotalItems());
	Ryan("CVector use: %d/%d\n", Mem::CPoolable<Script::CVector>::SGetNumUsedItems(), Mem::CPoolable<Script::CVector>::SGetTotalItems());

#if 1
//	uint32 crc_position 			= Script::GenerateCRC("position");
	uint32 crc_angles 				= Script::GenerateCRC("angles");
	uint32 crc_name 				= Script::GenerateCRC("name");
	uint32 crc_class 				= Script::GenerateCRC("class");
	uint32 crc_levelgeometry 		= Script::GenerateCRC("LevelGeometry");
	uint32 crc_createdatstart 		= Script::GenerateCRC("CreatedAtStart");
	uint32 crc_triggerscript 		= Script::GenerateCRC("TriggerScript");
	uint32 crc_trickobject 			= Script::GenerateCRC("TrickObject");
	uint32 crc_cluster 				= Script::GenerateCRC("Cluster");
	
	// 7
	CPiece *p_piece = mp_cloned_piece_list;
	while (p_piece)
	{
		CClonedPiece *p_cloned = p_piece->CastToCClonedPiece();
		if (!(p_cloned->GetFlags() & CPiece::mFLOOR) && 
			!(p_cloned->GetFlags() & CPiece::mRESTART))
		{
			
			Script::CScriptStructure *pNode = pNodeArray->GetStructure(*pNodeNum);
			Mth::Vector pos = p_cloned->GetPos();
			pNode->AddComponent(CRCD(0x7f261953,"pos"), pos.GetX(), pos.GetY(), pos.GetZ());
// GJ:  The following was incorrect, and the Z component should have been negated,
// but no one ever really used it, so we never noticed that it was a problem...
// The new-style 'pos' vector is already correct and needs no negation.
//			pNode->AddComponent(CRCD(0xb9d31b0a,"position"), pos.GetX(), pos.GetY(), pos.GetZ());
			pNode->AddComponent(crc_angles, 0.0f, 0.0f, 0.0f);
			// hopefully safe
			// XXX
			Ryan("id is 0x%x\n", p_cloned->m_id);
			pNode->AddComponent(crc_name, ESYMBOLTYPE_NAME, (int) p_cloned->m_sector_checksum);
			pNode->AddComponent(crc_class, ESYMBOLTYPE_NAME, (int) crc_levelgeometry);
			pNode->AddComponent(NONAME, ESYMBOLTYPE_NAME, (int) crc_createdatstart);
			
																				  
			pNode->AddComponent(NONAME, ESYMBOLTYPE_NAME, (int) crc_trickobject);
// Mick:  use the cloned pieces ID, but EOR with with 0xffffffff so it's different to the original name
			pNode->AddComponent(crc_cluster, ESYMBOLTYPE_NAME, (int) p_cloned->GetID() ^ 0xffffffff);

			
			// trigger script
			if (p_cloned->GetFlags() & CPiece::mGAP)
			{
				Dbg_Assert(CGapManager::sInstance());
				pNode->AddComponent(crc_triggerscript, ESYMBOLTYPE_NAME, (int) CGapManager::sInstance()->GetGapTriggerScript(p_cloned));
			}
			else if (p_cloned->GetSourcePiece()->m_triggerScriptId)
				pNode->AddComponent(crc_triggerscript, ESYMBOLTYPE_NAME, (int) p_cloned->GetSourcePiece()->m_triggerScriptId);
			
			(*pNodeNum)++;
		}
		p_piece = p_piece->mp_next_in_list;
	}
#endif
}

// Used by the rail editor for snapping to the nearest rail point.
bool CParkGenerator::FindNearestRailPoint(Mth::Vector &pos, Mth::Vector *p_nearest_pos, float *p_dist_squared)
{
	float min_dist_squared=100000000.0f;
	bool found_point=false;
	RailString *p_string = m_out_rail_set.GetList();
	
	while (p_string)
	{
		RailPoint *p_point = p_string->GetList();
		
		while (p_point)
		{
			Mth::Vector diff=p_point->m_pos-pos;
			float dd=diff.LengthSqr();
			
			if (dd < min_dist_squared)
			{
				min_dist_squared=dd;
				
				*p_nearest_pos=p_point->m_pos;
				*p_dist_squared=min_dist_squared;
				found_point=true;
			}
			
			p_point = p_point->GetNext();
		}
			
		p_string = p_string->GetNext();
	}

	return found_point;
}

// called from GenerateNodeInfo()
void CParkGenerator::set_up_rail_nodes(Script::CArray *pNodeArray, int *pNodeNum)
{

	Ryan("set_up_rail_nodes: %d/%d\n", Mem::CPoolable<Script::CComponent>::SGetNumUsedItems(), Mem::CPoolable<Script::CComponent>::SGetTotalItems());

#if 1	
	
	const bool debug_rail_out = false;
	
	uint32 crc_angles 			= Script::GenerateCRC("angles");
	uint32 crc_name 			= Script::GenerateCRC("name");
	uint32 crc_class 			= Script::GenerateCRC("class");
	uint32 crc_railnode 		= Script::GenerateCRC("railnode");
	uint32 crc_createdatstart 	= Script::GenerateCRC("CreatedAtStart");
	uint32 crc_links 			= Script::GenerateCRC("links");
	
//	uint32 crc_type				= Script::GenerateCRC("type");
//	uint32 crc_metal			= Script::GenerateCRC("metal");
//	uint32 crc_wood				= Script::GenerateCRC("wood");
	uint32 crc_terraintype		= Script::GenerateCRC("terraintype");
//	uint32 crc_terrain_wood		= Script::GenerateCRC("terrain_wood");
//	uint32 crc_terrain_metal	= Script::GenerateCRC("terrain_metal");
	
	uint32 crc_trickobject 		= Script::GenerateCRC("TrickObject");
	uint32 crc_cluster 			= Script::GenerateCRC("Cluster");
	
	/*
	Position = (-1619.978027,-336.000031,-6870.965332)
	Angles = (0.000000,-3.141589,0.000000)
	Name = TRG_Conc_Park_Rail0
	Class = RailNode
	Type = Concrete
	CreatedAtStart
	TerrainType = TERRAIN_CONCSMOOTH
	TrickObject Cluster = ParkingLotSpine1
	TriggerScript = CanTRG_Conc_Park_Rail0Script
	Links=[1]
	*/
	
	int num_points = m_out_rail_set.CountPoints();
	if (!num_points) return;

	RailString *pString = m_out_rail_set.GetList();
	RailPoint *pPoint = pString->GetList();
	
	if (debug_rail_out)
		Ryan("String %s: ", Script::FindChecksumName(pString->m_id));	
	int first_node_in_loop = *pNodeNum;
	
	// 10
	uint32 rail_point_id = vFIRST_ID_FOR_RAILS;
	int stop_node_num = *pNodeNum + num_points;
	for (; *pNodeNum < stop_node_num; (*pNodeNum)++)
	{
		Dbg_Assert(pPoint);
		
		Script::CStruct *pNode = pNodeArray->GetStructure(*pNodeNum);
		pNode->AddComponent(CRCD(0x7f261953,"pos"), pPoint->m_pos.GetX(), pPoint->m_pos.GetY(), pPoint->m_pos.GetZ());
//		pNode->AddComponent(CRCD(0xb9d31b0a,"Position"), pPoint->m_pos.GetX(), pPoint->m_pos.GetY(), pPoint->m_pos.GetZ());
		if (debug_rail_out)
			Ryan("[%.1f,%.1f,%.1f]", pPoint->m_pos.GetX(), pPoint->m_pos.GetY(), pPoint->m_pos.GetZ());
		pNode->AddComponent(crc_angles, 0.0f, 0.0f, 0.0f);
		Dbg_MsgAssert(rail_point_id < vMAX_ID_FOR_RAILS, ("out of rail ids"));
		pNode->AddComponent(crc_name, ESYMBOLTYPE_NAME, (int) rail_point_id++);
		pNode->AddComponent(crc_class, ESYMBOLTYPE_NAME, (int) crc_railnode);
		pNode->AddComponent(NONAME, ESYMBOLTYPE_NAME, (int) crc_createdatstart);

/*	
		uint32 type, terrain_type;

		
		if (pPoint->m_type == RailPoint::vWOOD)
		{
			type = crc_wood;
			terrain_type = crc_terrain_wood;
		}
		else
		{
			type = crc_metal;
			terrain_type = crc_terrain_metal;
		}
*/		

// Don't need type?		
//		pNode->AddComponent(crc_type, ESYMBOLTYPE_NAME, (int) type);
		
		pNode->AddComponent(crc_terraintype, ESYMBOLTYPE_NAME, (int) pPoint->m_type);
		
		// Mick: m_objectId will be zero if this rail is not part of a trickob
		if (pPoint->m_objectId)
		{
			pNode->AddComponent(NONAME, ESYMBOLTYPE_NAME, (int) crc_trickobject);
			pNode->AddComponent(crc_cluster, ESYMBOLTYPE_NAME, (int) pPoint->m_objectId ^ 0xffffffff);
		}
		
		if (pPoint->GetNext() != NULL || pString->m_isLoop)
		{
			int link_num = *pNodeNum + 1;
			if (pPoint->GetNext() == NULL && pString->m_isLoop)
				link_num = first_node_in_loop;
			
			Script::CArray *pLinks = new Script::CArray();
			pLinks->SetSizeAndType(1,ESYMBOLTYPE_INTEGER);
			pLinks->SetInteger(0, link_num);
			pNode->AddComponent(crc_links, pLinks);
			delete pLinks;
		}

		pPoint = pPoint->GetNext();
		if (pPoint)
		{
			if (debug_rail_out)
				Ryan("-");
		}
		else
		{
			if (debug_rail_out)
			{
				if (pString->m_isLoop) Ryan(" (loop)");
				Ryan("\n");
			}
			pString = pString->GetNext();
			if (pString)
			{
				if (debug_rail_out)
					Ryan("String %s: ", Script::FindChecksumName(pString->m_id));
				pPoint = pString->GetList();
				first_node_in_loop = *pNodeNum + 1;
			}
		}
	}
	
#endif	
}




// called from GenerateNodeInfo()
void CParkGenerator::set_up_restart_nodes(Script::CArray *pNodeArray, int *pNodeNum)
{
	uint32 crc_angles 			= Script::GenerateCRC("angles");
	uint32 crc_name 			= Script::GenerateCRC("name");
	uint32 crc_class 			= Script::GenerateCRC("class");
	uint32 crc_type 			= Script::GenerateCRC("type");
	uint32 crc_restart 			= Script::GenerateCRC("restart");
	uint32 crc_genericnode		= Script::GenerateCRC("GenericNode");
	uint32 crc_createdatstart 	= Script::GenerateCRC("CreatedAtStart");
	uint32 crc_restartname 		= Script::GenerateCRC("RestartName");
	uint32 crc_restart_types 	= Script::GenerateCRC("restart_types");
	uint32 crc_model		 	= Script::GenerateCRC("model");
	uint32 crc_triggerscript 	= Script::GenerateCRC("TriggerScript");
	
	uint32 restart_id = vFIRST_ID_FOR_RESTARTS;
	
	for (int i = 0; i < vNUM_RESTARTS; i++)
	{
		if (m_restart_tab[i].type != vEMPTY)
		{
			//Ryan("yo, restart at (%.2f,%.2f,%.2f)\n", m_restart_tab[i].pos.GetX(), m_restart_tab[i].pos.GetY(), m_restart_tab[i].pos.GetZ());
			
			// if restart is ONE_PLAYER or HORSE type, it will also be repeated 
			// as a multiplayer
			int repeat_num = 1;
			if (m_restart_tab[i].type == vHORSE)
				repeat_num = 2;
			if (m_restart_tab[i].type == vONE_PLAYER)
				repeat_num = 2;				

			for (int n = 0; n < repeat_num; n++)
			{
				/*
				{
					// Node 1965
					Position = (-476.991364,-70.263161,-5703.516602)
					Angles = (0.000000,0.000000,0.000000)
					Name = TRG_Crown01
					Class = GenericNode
					Type = Crown
					CreatedAtStart
				}
				{
					// Node 2912
					Position = (2940.254639,-1368.771729,-753.127625)
					Angles = (0.000000,2.687807,0.000000)
					Name = TRG_CTF_Restart_Red
					Class = Restart
					Type = CTF
					CreatedAtStart
					CollisionMode = BoundingBox
					RestartName = "Team: CTF"
					restart_types = [ CTF ]
				}
				
				*/
				
				
				int restart_name = restart_id++;		// use generic name for most restarts
				uint32 type_checksum = 0;
				if (m_restart_tab[i].type == vONE_PLAYER)
				{
					type_checksum = Script::GenerateCRC("Player1");
				}
				if (m_restart_tab[i].type == vHORSE)
				{
					type_checksum = Script::GenerateCRC("Horse");
				}
				if (m_restart_tab[i].type == vMULTIPLAYER || n == 1)
				{	
					type_checksum = Script::GenerateCRC("Multiplayer");
				}
				if (m_restart_tab[i].type == vKING_OF_HILL)
				{
					type_checksum = Script::GenerateCRC("Crown");
				}
				if (
					m_restart_tab[i].type == vRED_FLAG
					|| m_restart_tab[i].type == vGREEN_FLAG
					|| m_restart_tab[i].type == vBLUE_FLAG
					|| m_restart_tab[i].type == vYELLOW_FLAG
					)
				{
					type_checksum = Script::GenerateCRC("CTF");
					switch (m_restart_tab[i].type)
					{
						case vRED_FLAG:
							restart_name = Script::GenerateCRC("TRG_CTF_Restart_Red");
							break;
						case vGREEN_FLAG:
							restart_name = Script::GenerateCRC("TRG_CTF_Restart_Green");
							break;
						case vBLUE_FLAG:
							restart_name = Script::GenerateCRC("TRG_CTF_Restart_Blue");
							break;
						case vYELLOW_FLAG:
							restart_name = Script::GenerateCRC("TRG_CTF_Restart_Yellow");
							break;
						default:
							Dbg_MsgAssert(0,("Impossible!"));
							break;
					}
				}
				
				if (type_checksum != 0)
				{
	
					Script::CScriptStructure *pNode = pNodeArray->GetStructure(*pNodeNum);
					if (m_restart_tab[i].type == vKING_OF_HILL)
					{
						pNode->AddComponent(CRCD(0x7f261953,"pos"), m_restart_tab[i].pos.GetX(), m_restart_tab[i].pos.GetY() + 50.0f, m_restart_tab[i].pos.GetZ());
//						pNode->AddComponent(CRCD(0xb9d31b0a,"Position"), m_restart_tab[i].pos.GetX(), m_restart_tab[i].pos.GetY() + 50.0f, -m_restart_tab[i].pos.GetZ());
						pNode->AddComponent(crc_class, ESYMBOLTYPE_NAME, (int) crc_genericnode);
					}
					else
					{
						pNode->AddComponent(CRCD(0x7f261953,"pos"), m_restart_tab[i].pos.GetX(), m_restart_tab[i].pos.GetY() + 50.0f, m_restart_tab[i].pos.GetZ());
//						pNode->AddComponent(CRCD(0xb9d31b0a,"Position"), m_restart_tab[i].pos.GetX(), m_restart_tab[i].pos.GetY(), -m_restart_tab[i].pos.GetZ());
						pNode->AddComponent(crc_class, ESYMBOLTYPE_NAME, (int) crc_restart);
					}
					pNode->AddComponent(crc_angles, 0.0f, Mth::PI * 0.5f * ((float) m_restart_tab[i].dir + 2), 0.0f);
					pNode->AddComponent(crc_name, ESYMBOLTYPE_NAME, (int) restart_name);
					pNode->AddComponent(crc_type, ESYMBOLTYPE_NAME, (int) type_checksum);
					pNode->AddComponent(NONAME, ESYMBOLTYPE_NAME, (int) crc_createdatstart);
					if (m_restart_tab[i].type != vKING_OF_HILL)
					{
						char *p_name = "Multi Restart";
						if (n == 0)
						{
							switch (m_restart_tab[i].type)
							{
								case vONE_PLAYER:
									p_name = "Player 1 Restart";
									break;
								case vMULTIPLAYER:
									p_name = "Player 2 Restart";
									break;
								case vHORSE:
									p_name = "Horse Restart";
									break;
								case vRED_FLAG:
								case vGREEN_FLAG:
								case vBLUE_FLAG:
								case vYELLOW_FLAG:
									p_name = "Team: CTF";
									break;
								default:
									break;
									 
							}
						}						
						pNode->AddComponent(crc_restartname, ESYMBOLTYPE_STRING, p_name);
					
						Script::CArray *pRestartTypes = new Script::CArray();
						pRestartTypes->SetSizeAndType(1, ESYMBOLTYPE_NAME);
						pRestartTypes->SetChecksum(0, type_checksum);
						pNode->AddComponent(crc_restart_types, pRestartTypes);
						delete pRestartTypes;
					}
					//Script::PrintContents(pNode);								   
					(*pNodeNum)++;
				}
				
				// Flags have the 3 additional nodes for the team flags, bases, CTF flag
				
				if (
					m_restart_tab[i].type == vRED_FLAG
					|| m_restart_tab[i].type == vGREEN_FLAG
					|| m_restart_tab[i].type == vBLUE_FLAG
					|| m_restart_tab[i].type == vYELLOW_FLAG
					)
				{
					// Not a normal restart point, so must be a flag
					// so need to generate the flag, the CTF base and the CTF restart all in the same place

				/*					
				// team flag is like:
				{
					// Node 2678
					Position = (864.618835,-408.157806,-3771.989014)
					Angles = (0.000000,2.268928,0.000000)
					Name = TRG_Flag_Red
					Class = GameObject
					Type = Flag_Red
					Model = "gameobjects\flags\flag_red\flag_red.mdl"
					SuspendDistance = 0
					lod_dist1 = 800
					lod_dist2 = 801
					TriggerScript = TRG_Flag_RedScript
				}
				
				then pretty much the same for CTF flags, just a different name
				
				{
					// Node 3360
					Position = (4060.947998,408.885132,1926.812012)
					Angles = (0.000000,0.000000,0.000000)
					Name = TRG_CTF_Blue						<<<<<<<<<<<<<<   Different name
					Class = GameObject
					Type = Flag_Blue
					Model = "gameobjects\flags\flag_blue\flag_blue.mdl"
					SuspendDistance = 0
					lod_dist1 = 800
					lod_dist2 = 801
					TriggerScript = TRG_CTF_BlueScript		<<<<<<<<<<<<<<  script can actually stay the same
				}
				
				and then the base
				{
					// Node 3252
					Position = (-2448.872314,-373.527405,5749.742188)
					Angles = (0.000000,-2.792527,0.000000)
					Name = TRG_CTF_Yellow_Base		 <<<<<<<<<<<<<<<<<<<<<<<<<<<<  different name
					Class = GameObject
					Type = Flag_Yellow_Base			   <<<<<<<<<<<<<<<<<<<<<<<<<<<  different type
					Model = "gameobjects\flags\flag_yellow_base\flag_yellow_base.mdl"   <<<<<< differnt model
					SuspendDistance = 0
					lod_dist1 = 800
					lod_dist2 = 801
													<<<<<<<<<<<<<<<  no script
				}


				
				*/	
				
	
					Script::CScriptStructure *pNode = pNodeArray->GetStructure(*pNodeNum);

					const	char *color = NULL;					
					if (m_restart_tab[i].type == vRED_FLAG) color = "red";
					if (m_restart_tab[i].type == vGREEN_FLAG) color = "green";
					if (m_restart_tab[i].type == vBLUE_FLAG) color = "blue";
					if (m_restart_tab[i].type == vYELLOW_FLAG) color = "yellow";

					char name_string[256];
					char type_string[256];
					char model_string[256];
					char triggerscript_string[256];
					sprintf(type_string,"Flag_%s",color);
					sprintf(model_string,"gameobjects\\flags\\flag_%s\\flag_%s.mdl",color,color);
					sprintf(triggerscript_string,"TRG_Flag_%sScript_Parked",color);
					
					sprintf(name_string,"TRG_Flag_%s",color);
					pNode->AddComponent(CRCD(0x7f261953,"pos"), m_restart_tab[i].pos.GetX(), m_restart_tab[i].pos.GetY(), m_restart_tab[i].pos.GetZ());
//					pNode->AddComponent(CRCD(0xb9d31b0a,"Position"), m_restart_tab[i].pos.GetX(), m_restart_tab[i].pos.GetY(), -m_restart_tab[i].pos.GetZ());
					pNode->AddComponent(crc_angles, 0.0f, Mth::PI * 0.5f * ((float) m_restart_tab[i].dir + 2), 0.0f);
					pNode->AddComponent(crc_name, ESYMBOLTYPE_NAME, Script::GenerateCRC(name_string));
					pNode->AddComponent(crc_class, ESYMBOLTYPE_NAME, Script::GenerateCRC("GameObject"));
					pNode->AddComponent(crc_type, ESYMBOLTYPE_NAME, Script::GenerateCRC(type_string));
					pNode->AddComponent(crc_model, ESYMBOLTYPE_STRING, model_string);
					pNode->AddComponent(crc_triggerscript, ESYMBOLTYPE_NAME, Script::GenerateCRC(triggerscript_string));

					pNode->AddInteger("nodeIndex", *pNodeNum);
					pNode->AddInteger("SuspendDistance", 0);
					pNode->AddInteger("lod_dist1", 800);
					pNode->AddInteger("lod_dist2", 801);
//					Script::PrintContents(pNode);								   
					(*pNodeNum)++;				
					
					// then repeat exactly for the CTF node, with different name
					
					pNode = pNodeArray->GetStructure(*pNodeNum);
					sprintf(name_string,"TRG_CTF_%s",color);
					pNode->AddComponent(CRCD(0x7f261953,"pos"), m_restart_tab[i].pos.GetX(), m_restart_tab[i].pos.GetY(), m_restart_tab[i].pos.GetZ());
//					pNode->AddComponent(CRCD(0xb9d31b0a,"Position"), m_restart_tab[i].pos.GetX(), m_restart_tab[i].pos.GetY(), -m_restart_tab[i].pos.GetZ());
					pNode->AddComponent(crc_angles, 0.0f, Mth::PI * 0.5f * ((float) m_restart_tab[i].dir + 2), 0.0f);
					pNode->AddComponent(crc_name, ESYMBOLTYPE_NAME, Script::GenerateCRC(name_string));
					pNode->AddComponent(crc_class, ESYMBOLTYPE_NAME, Script::GenerateCRC("GameObject"));
					pNode->AddComponent(crc_type, ESYMBOLTYPE_NAME, Script::GenerateCRC(type_string));
					pNode->AddComponent(crc_model, ESYMBOLTYPE_STRING, model_string);
					pNode->AddComponent(crc_triggerscript, ESYMBOLTYPE_NAME, Script::GenerateCRC(triggerscript_string));

					pNode->AddInteger("nodeIndex", *pNodeNum);
					pNode->AddInteger("SuspendDistance", 0);
					pNode->AddInteger("lod_dist1", 800);
					pNode->AddInteger("lod_dist2", 801);
//					Script::PrintContents(pNode);								   
					(*pNodeNum)++;				

					// different for base (note, no triggerscript)
					pNode = pNodeArray->GetStructure(*pNodeNum);
					sprintf(name_string,"TRG_CTF_%s_Base",color);
					sprintf(type_string,"Flag_%s_Base",color);
					sprintf(model_string,"gameobjects\\flags\\flag_%s_base\\flag_%s_base.mdl",color,color);
					
					pNode->AddComponent(CRCD(0x7f261953,"pos"), m_restart_tab[i].pos.GetX(), m_restart_tab[i].pos.GetY(), m_restart_tab[i].pos.GetZ());
//					pNode->AddComponent(CRCD(0xb9d31b0a,"Position"), m_restart_tab[i].pos.GetX(), m_restart_tab[i].pos.GetY(), -m_restart_tab[i].pos.GetZ());
					pNode->AddComponent(crc_angles, 0.0f, Mth::PI * 0.5f * ((float) m_restart_tab[i].dir + 2), 0.0f);
					pNode->AddComponent(crc_name, ESYMBOLTYPE_NAME, Script::GenerateCRC(name_string));
					pNode->AddComponent(crc_class, ESYMBOLTYPE_NAME, Script::GenerateCRC("GameObject"));
					pNode->AddComponent(crc_type, ESYMBOLTYPE_NAME, Script::GenerateCRC(type_string));
					pNode->AddComponent(crc_model, ESYMBOLTYPE_STRING, model_string);

					pNode->AddInteger("nodeIndex", *pNodeNum);
					pNode->AddInteger("SuspendDistance", 0);
					pNode->AddInteger("lod_dist1", 800);
					pNode->AddInteger("lod_dist2", 801);
//					Script::PrintContents(pNode);								   
					(*pNodeNum)++;				
				}
			} // end for n
		} // end if
	}

/*	
	Position = (2456.617188,-218.955109,-6663.688477)
	Angles = (0.741766,-1.570794,-0.000002)
	Name = TRG_Restart_Parking_Lot04
	Class = Restart
	Type = Player1
	CreatedAtStart
	RestartName = "P1: Restart"
	restart_types =
	[
	Player1
	]
*/
}


////////////////////////////////////////////////////////////////////////////
// Restart stuff

/* 
	Should be called from 
	-CParkManager::RebuildNodeArray() (was in Map::GenerateWorld())
	-AddMetaPieceToPark::AddMetaPieceToPark() (was in Map::AddPiece())
*/
// Add a restart point if space
// Non-automatic restart points will overwrite automatic restart points
// (but not if they find an empty slot first)
bool CParkGenerator::AddRestart(Mth::Vector &pos, int dir, GridDims &dims, RestartType type, bool automatic, bool auto_copy)
{
	int first_slot = 0, last_slot = 0;
	get_restart_slots(type, &first_slot, &last_slot);

	Mth::Vector	use_pos = pos;
	GridDims use_dims = dims;

	// When "auto_copy" is set
	// If this is an automatic point, then copy the position
	// from the last non-automatic point we find
	if (automatic && auto_copy)
	{
		for (int i = first_slot; i <= last_slot; i++)
		{
			if (m_restart_tab[i].type == type && !m_restart_tab[i].automatic)
			{
				use_pos = m_restart_tab[i].pos;				
				use_dims = m_restart_tab[i].dims;				
				#ifdef	DEBUG_RESTARTS
				printf ("%d: restart copying position from slot %d",__LINE__,i);
				#endif
			}
		}
	}
	
	
	
	for (int i = first_slot; i <= last_slot; i++)
	{
		
		// add the restart to any empty slot, or overwrite an "automatic" slot if this is not automatic 
		if (m_restart_tab[i].type == vEMPTY || (m_restart_tab[i].automatic && !automatic))		
		{
			m_restart_tab[i].pos = use_pos;
			m_restart_tab[i].dir = dir;
			m_restart_tab[i].dims = use_dims;
			m_restart_tab[i].type = type;
			m_restart_tab[i].automatic = automatic;
			#ifdef	DEBUG_RESTARTS
			printf ("%d: restart (%.2f,%.2f,%.2f) set in slot %d  (type = %d, auto = %d\n)",__LINE__,use_pos[X],use_pos[Y],use_pos[Z],i,type,automatic);
			#endif
			return true;
		}		
	}
	
	return false;
}


int CParkGenerator::NumRestartsOfType(RestartType type)
{
	int first_slot = 0, last_slot = 0;
	get_restart_slots(type, &first_slot, &last_slot);

	int got = 0;									// count of number of actual restarts
	for (int i = first_slot; i <= last_slot; i++)
	{
		if (m_restart_tab[i].type != vEMPTY)		// got some kind of restart
		{
				got++; 								// counting an actual restart
		}
	}
	return got;
}


// should be called from CParkManager::RebuildNodeArray() (was in Map::GenerateWorld())
// returns true if there are not enough restart points (ie, true if we need more)
// Note:  we include "automatic" restart points now.
bool CParkGenerator::NotEnoughRestartsOfType(RestartType type, int need)
{
	return NumRestartsOfType(type) < need;		  // if got less than we need, return true, (we need more)
}


Mth::Vector	 CParkGenerator::GetRestartPos(RestartType type, int index)
{

	int first_slot = 0, last_slot = 0;
	get_restart_slots(type, &first_slot, &last_slot);

	for (int i = first_slot; i <= last_slot; i++)
	{
		if (m_restart_tab[i].type != vEMPTY)		// got some kind of restart
		{
			index--;								
			if (index < 0)
			{
				return m_restart_tab[i].pos;
			}
		}
	}
		
	Dbg_MsgAssert(0,("could not find restart pos"));	
	return Mth::Vector(0,0,0);

}

GridDims	 CParkGenerator::GetRestartDims(RestartType type, int index)
{

	int first_slot = 0, last_slot = 0;
	get_restart_slots(type, &first_slot, &last_slot);

	for (int i = first_slot; i <= last_slot; i++)
	{
		if (m_restart_tab[i].type != vEMPTY)		// got some kind of restart
		{
			index--;								
			if (index < 0)
			{
				return m_restart_tab[i].dims;
			}
		}
	}
		
	Dbg_MsgAssert(0,("could not find restart dims"));	
	return	GridDims();

}



// called by various restart functions in this class
void CParkGenerator::get_restart_slots(RestartType type, int *pFirstSlot, int *pLastSlot)
{
	switch (type)
	{
		case vONE_PLAYER:
			*pFirstSlot = 1;
			*pLastSlot = 1;
			break;
		case vMULTIPLAYER:
			*pFirstSlot = 0;
			*pLastSlot = 0;
			break;
		case vHORSE:
			*pFirstSlot = 2;
			*pLastSlot = 7;
			break;
		case vKING_OF_HILL:
			*pFirstSlot = 8;
			*pLastSlot = 13;
			break;
		case vRED_FLAG:	
			*pFirstSlot = 14;
			*pLastSlot = 14;
			break;
		case vGREEN_FLAG:	
			*pFirstSlot = 15;
			*pLastSlot = 15;
			break;
		case vBLUE_FLAG:	
			*pFirstSlot = 16;
			*pLastSlot = 16;
			break;
		case vYELLOW_FLAG:	
			*pFirstSlot = 17;
			*pLastSlot = 17;
			break;
		default:
			Dbg_MsgAssert(0,("Unhandled restart type %d",type));
			break;
	}
}




// should be called by CParkManager::DestroyConcreteMeta() (was Map::RemovePiece())
void CParkGenerator::RemoveRestart(const GridDims &dims, RestartType type)
{
	int first_slot = 0, last_slot = 0;
	get_restart_slots(type, &first_slot, &last_slot);
	
	for (int i = first_slot; i <= last_slot; i++)
	{
//		if (m_restart_tab[i].area.x == area.x &&
//			m_restart_tab[i].area.y == area.y &&
//			m_restart_tab[i].area.z == area.z)
		if (
			m_restart_tab[i].dims.GetX() == dims.GetX()
			&& m_restart_tab[i].dims.GetY() == dims.GetY()
			&& m_restart_tab[i].dims.GetZ() == dims.GetZ()
			)
		{
			m_restart_tab[i].type = vEMPTY;
		}
	}
}




// should be called from CCursor::AttemptStamp() (was Cursor::AttemptPlacePiece())
bool CParkGenerator::FreeRestartExists(RestartType type)
{
	int first_slot = 0, last_slot = 0;
	get_restart_slots(type, &first_slot, &last_slot);
	
	for (int i = first_slot; i <= last_slot; i++)
		if (m_restart_tab[i].type == vEMPTY || m_restart_tab[i].automatic)
		{
//			m_restart_tab[i].type = vEMPTY;
			return true;
		}
	return false;
}




/* 
	should be called from:
	-CParkGenerator::InitializeMasterPieces() (was World::CreateParkHeap())
	-CParkGenerator::DestroyAllPieces() or CParkManager::destroy_concrete_metapieces() (was World::DestroyAllPieces())
*/
void CParkGenerator::KillRestarts()
{
	#ifdef	DEBUG_RESTARTS
	printf ("%d: KillRestarts called, all restarts set to vEMPTY",__LINE__);
	#endif

	for (int i = 0; i < vNUM_RESTARTS; i++)
		m_restart_tab[i].type = vEMPTY;
}


void CParkGenerator::ClearAutomaticRestarts()
{

	#ifdef	DEBUG_RESTARTS
	printf ("%d: ClearAutomaticRestarts called, all automatic restarts set to vEMPTY",__LINE__);
	#endif
	
	for (int i = 0; i < vNUM_RESTARTS; i++)
	{
		if (m_restart_tab[i].automatic)
		{
			m_restart_tab[i].type = vEMPTY;
		}
	}
}




/*
	Left edge of gap is at position pos. Bottom edge sticks out in direction
	indicated by rot, for length units.
*/
CClonedPiece *CParkGenerator::CreateGapPiece(Mth::Vector &pos, float length, int rot)
{
	// maken der gapen piece
	CClonedPiece *p_gap_piece = ClonePiece(GetMasterPiece(Script::GenerateCRC("Sk4Ed_Gap_10x10"), true), CPiece::mNO_FLAGS);
	p_gap_piece->SetDesiredPos(pos, CClonedPiece::CHANGE_SECTOR);
	p_gap_piece->SetDesiredRot(Mth::ERot90(rot), CClonedPiece::CHANGE_SECTOR);
	p_gap_piece->set_flag(CPiece::mGAP);
	AddClonedPieceToWorld(p_gap_piece);
	// XXX
	Ryan("@@@ made gap piece 0x%x\n", p_gap_piece);
	return p_gap_piece;
}




}


