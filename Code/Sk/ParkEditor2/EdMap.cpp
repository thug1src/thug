#include <core/defines.h>
#include <sk/ParkEditor/EdRail.h>
#include <sk/ParkEditor2/EdMap.h>
#include <sk/ParkEditor2/ParkEd.h>
#include <sk/ParkEditor2/clipboard.h>
#include <core/math.h>
#include <core/crc.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/symboltype.h>
#include <gel/scripting/component.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/utils.h>
#include <gel/music/music.h>
#include <sys/file/filesys.h>
#include <sk/gamenet/gamenet.h>
#include <sk/components/raileditorcomponent.h>
#include <sk/components/goaleditorcomponent.h>
#include <sk/engine/feeler.h>

DefinePoolableClass(Ed::CMapListNode);

#ifdef __PLAT_NGC__
#define _16(a) (((a>>8)&0x00ff)|((a<<8)&0xff00))
#define _32(a) (((a>>24)&0x000000ff)|((a>>8)&0x0000ff00)|((a<<8)&0x00ff0000)|((a<<24)&0xff000000)) 
#else
#define _16(a) a
#define _32(a) a
#endif		// __PLAT_NGC__

#if 1
#ifdef	__PLAT_NGPS__
#define	__USE_EXTERNAL_BUFFER__
#include <gfx/nx.h>
#include <gfx/ngps/nx/dma.h>
#define	__EXTERNAL_BUFFER__ NxPs2::dma::pRuntimeBuffer
#define	 ACQUIRE_BUFFER Nx::CEngine::sFinishRendering();
#endif
#endif

namespace Ed
{




DefineSingletonClass(CParkManager, "Park Manager");



CParkManager *CParkManager::sp_instance = NULL;

GridDims::GridDims(uint8 x, sint8 y, uint8 z, uint8 w, uint8 h, uint8 l)
{
	m_dims[0] = x;
	m_dims[1] = (uint8) y;
	m_dims[2] = z;
	m_dims[3] = w;
	m_dims[4] = h;
	m_dims[5] = l;
}




uint8 &GridDims::operator [](sint i)
{
	Dbg_Assert(i < 6);
	return m_dims[i];
}




// Effectively gives the area an H of infinity
void GridDims::MakeInfinitelyHigh()
{
	m_dims[4] = 127 - (sint8) m_dims[1];
}




// Effectively gives the area an H of infinity
void GridDims::MakeInfiniteOnY()
{
	m_dims[1] = (uint8) -100;
	m_dims[4] = 200;
}




void GridDims::PrintContents() const
{
	printf("pos=(%d,%d,%d) dims=(%d,%d,%d)\n", 
		   m_dims[0], (sint8) m_dims[1], m_dims[2],
		   m_dims[3], m_dims[4], m_dims[5]);
}




CMetaPiece::CMetaPiece()
{
	m_flags = EFlags(0);
	mp_additional_desc_tab = NULL;
	m_total_entries = 0;
	m_num_used_entries = 0;
}




CMetaPiece::~CMetaPiece()
{
	if (mp_additional_desc_tab)
		delete mp_additional_desc_tab;
}




CConcreteMetaPiece *CMetaPiece::CastToConcreteMeta()
{
	Dbg_MsgAssert(this == NULL || (m_flags & mCONCRETE_META), ("not a concrete metapiece"));
	return (CConcreteMetaPiece *) this;
}




CAbstractMetaPiece *CMetaPiece::CastToAbstractMeta()
{
	Dbg_MsgAssert(this == NULL || (m_flags & mABSTRACT_META), ("not an abstract metapiece"));
	return (CAbstractMetaPiece *) this;
}

bool CMetaPiece::IsRestartOrFlag()
{
	switch (m_name_checksum)
	{
		case CRCC(0xdc09a6a4,"Sk3Ed_RS_1p"):
		case CRCC(0xca07b4fc,"Sk3Ed_RS_Mp"):
		case CRCC(0x3a784d4c,"Sk3Ed_Rs_Ho"):
		case CRCC(0x4e2efd2a,"Sk3Ed_Rs_KOTH"):
		case CRCC(0x613c7766,"Sk4Ed_Team_Yellow"):
		case CRCC(0xec82433c,"Sk4Ed_Team_Green"):
		case CRCC(0xd7eb62b5,"Sk4Ed_Team_Red"):
		case CRCC(0x8a1576b6,"Sk4Ed_Team_Blue"):
			return true;
			break;
			
		default:
			break;
	}		
	
	return false;
}

bool CMetaPiece::IsCompletelyWithinArea(const GridDims &area)
{
	if (m_cell_area.GetX() >= area.GetX() && m_cell_area.GetX()+m_cell_area.GetW() <= area.GetX()+area.GetW() &&
		m_cell_area.GetZ() >= area.GetZ() && m_cell_area.GetZ()+m_cell_area.GetL() <= area.GetZ()+area.GetL())
	{
		return true;
	}
	return false;	
}

/*
	Use to set rotation of abstract metapiece for testing purposes (in GetMetaPiecesAt()), or
	to set rotation of concrete metapiece before adding to park.
*/
void CMetaPiece::SetRot(Mth::ERot90 newRot)
{
	Dbg_MsgAssert(!(m_flags & CMetaPiece::mIN_PARK), ("metapiece already in park -- can't change rotation"));
	
	// just want to change relative to current rotation
	Mth::ERot90 rot = Mth::ERot90((newRot + 4 - m_rot) & 3);
	m_rot = newRot;
	if (rot & 1)
	{
		// switch W and L
		m_cell_area.SetWHL(m_cell_area.GetL(), m_cell_area.GetH(), m_cell_area.GetW());
	}
	
	SMetaDescriptor temp_new_first[USUAL_NUM_DESCRIPTORS];
	
	// Create a new table, which will soon replace the old one
	SMetaDescriptor *p_meta_tab = NULL;
	if (mp_additional_desc_tab)
	{
		Mem::Manager::sHandle().PushContext(CParkManager::sInstance()->GetGenerator()->GetParkEditorHeap());
		p_meta_tab = new SMetaDescriptor[m_total_entries-USUAL_NUM_DESCRIPTORS];
		Mem::Manager::sHandle().PopContext();
	}
	for (uint i = 0; i < m_num_used_entries; i++)
	{
		// "dims" contains position of upper-left, after rotation
		SMetaDescriptor &out = (i < USUAL_NUM_DESCRIPTORS) ? temp_new_first[i] : p_meta_tab[i-USUAL_NUM_DESCRIPTORS];
		SMetaDescriptor &in = (i < USUAL_NUM_DESCRIPTORS) ? m_first_desc_tab[i] : mp_additional_desc_tab[i-USUAL_NUM_DESCRIPTORS];
		out = in;

		// dimensions with current rotation (not new)
		// OOG!
		GridDims in_dims(0, 0, 0);
		if (m_flags & mABSTRACT_META)
		{
			CAbstractMetaPiece *p_meta = CParkManager::sInstance()->GetAbstractMeta(in.mPieceName);
			in_dims = p_meta->GetArea();
			if ((in.mRot & 1))
			{
				// for a concrete metapiece, the retrieved cell dimensions will already reflect
				// the current rotation, so no need to enter this block
				in_dims.SetWHL(in_dims.GetL(), in_dims.GetH(), in_dims.GetW()); // switch W & L
			}
		}
		else
		{
			in.mpPiece->GetCellDims(&in_dims);
		}

		switch(rot)
		{
			case Mth::ROT_0:
				out.mX = in.mX;
				out.mZ = in.mZ;
				break;
			case Mth::ROT_90:
				out.mX = in.mZ;
				out.mZ = m_cell_area.GetL() - in.mX - in_dims.GetW();
				break;
			case Mth::ROT_180:
				out.mX = m_cell_area.GetW() - in.mX - in_dims.GetW();
				out.mZ = m_cell_area.GetL() - in.mZ - in_dims.GetL();
				break;
			case Mth::ROT_270:
				out.mX = m_cell_area.GetW() - in.mZ - in_dims.GetL();
				out.mZ = in.mX;
				break;
			default:
				Dbg_MsgAssert(0, ("ahhh-oooop?"));
				break;
		}		
		
		out.mY = in.mY;
		out.mRot = Mth::ERot90((in.mRot + rot) & 3);
		
		// this will also copy in.mPieceName, if this is an abstract metapiece
		out.mpPiece = in.mpPiece;
	} // end for

	if (mp_additional_desc_tab)
		delete mp_additional_desc_tab;
	mp_additional_desc_tab = p_meta_tab;
	for (int i = 0; i < USUAL_NUM_DESCRIPTORS; i++)
	{
		m_first_desc_tab[i] = temp_new_first[i];
		//printf("desc at (%d,%d,%d)\n", m_first_desc_tab[i].mX, m_first_desc_tab[i].mY, m_first_desc_tab[i].mZ);
	}
}




// returns postion of piece's center, relative to center of metapiece
Mth::Vector CMetaPiece::GetRelativePosOfContainedPiece(int index)
{
	// center_pos in world coordinates
	Mth::Vector center_pos;
	center_pos[X] = (float) m_cell_area.GetW() * CParkGenerator::CELL_WIDTH / 2.0f;
	center_pos[Y] = 0;
	center_pos[Z] = (float) m_cell_area.GetL() * CParkGenerator::CELL_LENGTH / 2.0f;

	SMetaDescriptor &desc = get_desc_at_index(index);	
	CPiece *p_piece = GetContainedPiece(index);
	Mth::Vector piece_pos(desc.mX * CParkGenerator::CELL_WIDTH + p_piece->GetDimsWithRot(Mth::ERot90(desc.mRot)).GetX() / 2.0f,
						  desc.mY * CParkGenerator::CELL_HEIGHT,
						  desc.mZ * CParkGenerator::CELL_LENGTH + p_piece->GetDimsWithRot(Mth::ERot90(desc.mRot)).GetZ() / 2.0f);
	return piece_pos - center_pos;
}




void CMetaPiece::BuildElement3dSectorsArray(Script::CArray *pArray)
{
	pArray->SetSizeAndType(m_num_used_entries, ESYMBOLTYPE_STRUCTURE);
	for (uint i = 0; i < m_num_used_entries; i++)
	{
		SMetaDescriptor &desc = get_desc_at_index(i);	
		
		Script::CStruct *p_struct = new Script::CStruct();
		p_struct->AddChecksum(NONAME, desc.mPieceName);
		Mth::Vector pos = GetRelativePosOfContainedPiece(i);
		p_struct->AddVector(NONAME, pos.GetX(), pos.GetY(), pos.GetZ());
		
		pArray->SetStructure(i, p_struct);
	}
}




/*
	Creates	new table of SMetaDescriptors. Might copy old one.
*/
void CMetaPiece::initialize_desc_table(int numEntries)
{
	SMetaDescriptor *p_new_tab = NULL;
	if (numEntries > USUAL_NUM_DESCRIPTORS) 
	{
		Mem::Manager::sHandle().PushContext(CParkManager::sInstance()->GetGenerator()->GetParkEditorHeap());
		p_new_tab = new SMetaDescriptor[numEntries-USUAL_NUM_DESCRIPTORS];
		Mem::Manager::sHandle().PopContext();
	}

	// Copy old table, if any. Only copied if there are to be at least as many entries as
	// there were before.
	if (p_new_tab && mp_additional_desc_tab && m_total_entries <= (uint) numEntries)
	{
		for (uint i = USUAL_NUM_DESCRIPTORS; i < m_total_entries; i++)
			p_new_tab[i-USUAL_NUM_DESCRIPTORS] = mp_additional_desc_tab[i-USUAL_NUM_DESCRIPTORS];
	}
	
	if (mp_additional_desc_tab)
		delete mp_additional_desc_tab;
	mp_additional_desc_tab = p_new_tab; 
	m_total_entries = numEntries;
}
								



/*
	Returns specified descriptor.
*/
SMetaDescriptor &CMetaPiece::get_desc_at_index(int i)
{
	if (i < USUAL_NUM_DESCRIPTORS) return m_first_desc_tab[i];

	Dbg_MsgAssert(mp_additional_desc_tab && (uint) i < m_num_used_entries, ("descriptor entry doesn't exist"));
	return mp_additional_desc_tab[i-USUAL_NUM_DESCRIPTORS];
}




CConcreteMetaPiece::CConcreteMetaPiece()
{
	m_flags = EFlags(m_flags | mCONCRETE_META);
	m_rot = 0;
}




int	CConcreteMetaPiece::CountContainedPieces()
{
	return m_num_used_entries;
}




CPiece *CConcreteMetaPiece::GetContainedPiece(int index)
{
	if ((uint) index >= m_num_used_entries || index < 0)
		return NULL;

	return get_desc_at_index(index).mpPiece;
}




/*
	Used for the cursor piece, allowing any position and rotation, not just cell-aligned with 90 degree
	increments. Doesn't affect "hard" position, rotation.
*/
void CConcreteMetaPiece::SetSoftRot(Mth::Vector pos, float rot)
{
	//Ryan("moving metapiece to (%.2f,%.2f,%.2f)\n", pos[X], pos[Y], pos[Z]);
	
	Mth::Vector center(m_cell_area.GetW() * CParkGenerator::CELL_WIDTH / 2.0f,
					   0.0f,
					   m_cell_area.GetL() * CParkGenerator::CELL_LENGTH / 2.0f);
	
	for (uint i = 0; i < m_num_used_entries; i++)
	{
		SMetaDescriptor &desc = get_desc_at_index(i);
		
		// BAKU
		Mth::Vector piece_dims = desc.mpPiece->GetDims();
		Mth::Vector piece_pos(desc.mX * CParkGenerator::CELL_WIDTH + piece_dims[X] / 2.0f - center[X],
							  desc.mY * CParkGenerator::CELL_HEIGHT - center[Y],
							  desc.mZ * CParkGenerator::CELL_LENGTH + piece_dims[Z] / 2.0f - center[Z]);

		Mth::Matrix rot_mat;
		rot_mat.Ident();
		rot_mat.SetPos(piece_pos);
		rot_mat.RotateY(Mth::DegToRad(rot));
		rot_mat.Translate(pos);

		//Ryan("    sub-piece position (%d) (%.2f,%.2f,%.2f)\n", desc.mY, rot_mat[Mth::POS][X], rot_mat[Mth::POS][Y], rot_mat[Mth::POS][Z]);
		
		desc.mpPiece->CastToCClonedPiece()->SetDesiredPos(rot_mat[Mth::POS], CClonedPiece::CHANGE_SECTOR);		
		//desc.mpPiece->CastToCClonedPiece()->SetDesiredRot(Mth::ROT_0, CClonedPiece::CHANGE_SECTOR);
		
		float soft_rot = 90.0f * (float) desc.mRot + rot;
		desc.mpPiece->CastToCClonedPiece()->SetSoftRot(soft_rot);
	}
}


// Turn on or off the highlight effect for this concrete meta piece
// which just turns the effect on or off for the cloned pieces that make it up 
void CConcreteMetaPiece::Highlight(bool on, bool gapHighlight)
{
	for (uint i = 0; i < m_num_used_entries; i++)
	{
		SMetaDescriptor &desc = get_desc_at_index(i);
		if (desc.mpPiece->GetFlags() & CPiece::mCLONED_PIECE)
		{
			desc.mpPiece->CastToCClonedPiece()->Highlight(on, gapHighlight);
		}	
	}
}



void CConcreteMetaPiece::SetVisibility(bool visible)
{
	for (uint i = 0; i < m_num_used_entries; i++)
	{
		SMetaDescriptor &desc = get_desc_at_index(i);
		desc.mpPiece->CastToCClonedPiece()->SetActive(visible);
	}
}




/*
	Use after calls to add_piece() have been made. Once metapiece has been locked, it can be added to 
	the world or used as a cursor
*/
void CConcreteMetaPiece::lock()
{
	// work out dimensions of metapiece from contained pieces
	Mth::Vector min_pos, max_pos;
	get_max_min_world_coords(max_pos, min_pos);
	
	Mth::Vector my_dims;
	my_dims.Set(max_pos.GetX() - min_pos.GetX(), max_pos.GetY() - min_pos.GetY(), max_pos.GetZ() - min_pos.GetZ());

	m_cell_area.SetWHL((uint8) ((my_dims.GetX() + 2.0f) / CParkGenerator::CELL_WIDTH),
					   (uint8) ((my_dims.GetY() + 2.0f) / CParkGenerator::CELL_HEIGHT),
					   (uint8) ((my_dims.GetZ() + 2.0f) / CParkGenerator::CELL_LENGTH));

	if (m_cell_area[H] < 1) m_cell_area[H] = 1;
	
	//Dbg_Assert(m_cell_area.mW > 0 && m_cell_area.mL > 0);

	// reposition all contained pieces relative to center bottom
	for (uint i = 0; i < m_num_used_entries; i++)
	{
		CClonedPiece *p_piece = get_desc_at_index(i).mpPiece->CastToCClonedPiece();
		
		//Mth::Vector piece_dims = p_piece->GetDims();
		p_piece->m_pos[X] = p_piece->m_pos.GetX() - min_pos.GetX() - my_dims.GetX() / 2.0f;
		p_piece->m_pos[Z] = p_piece->m_pos.GetZ() - min_pos.GetZ() - my_dims.GetZ() / 2.0f;
		p_piece->m_pos[Y] = p_piece->m_pos.GetY(); // - min_pos.GetY();
	}
}




/*
	Called by lock()
*/
void CConcreteMetaPiece::get_max_min_world_coords(Mth::Vector &max, Mth::Vector &min)
{
	min.Set(100000.0f, 100000.0f, 100000.0f, 0.0f);
	max.Set(-100000.0f, -100000.0f, -100000.0f, 0.0f);

	for (uint i = 0; i < m_num_used_entries; i++)
	{
		CClonedPiece *p_piece = get_desc_at_index(i).mpPiece->CastToCClonedPiece();
		
		Mth::Vector piece_dims = p_piece->GetDims();
		Mth::Vector piece_pos = p_piece->GetPos();

		if (piece_pos.GetX() - piece_dims.GetX() / 2.0f < min.GetX())
			min[X] = piece_pos.GetX() - piece_dims.GetX() / 2.0f;
		if (piece_pos.GetX() + piece_dims.GetX() / 2.0f > max.GetX())
			max[X] = piece_pos.GetX() + piece_dims.GetX() / 2.0f;
		
		if (piece_pos.GetZ() - piece_dims.GetZ() / 2.0f < min.GetZ())
			min[Z] = piece_pos.GetZ() - piece_dims.GetZ() / 2.0f;
		if (piece_pos.GetZ() + piece_dims.GetZ() / 2.0f > max.GetZ())
			max[Z] = piece_pos.GetZ() + piece_dims.GetZ() / 2.0f;
		
		if (piece_pos.GetY() < min.GetY())
			min[Y] = piece_pos.GetY();
		if (piece_pos.GetY() + piece_dims.GetY() > max.GetY())
			max[Y] = piece_pos.GetY() + piece_dims.GetY();
	}

	//Dbg_Assert(max[X] - min[X] > 1.0f && max[Z] - min[Z] > 1.0f);
}




/* 
	Determines if overlap between specified area of park and this metapiece.
	
	If excludeRisers set, then we ignore overlaps with contained CPieces that are risers.
*/
bool CConcreteMetaPiece::test_for_conflict(GridDims area, bool excludeRisers)
{
	// for now, we simply see if this metapiece overlaps with the area
	// later, we may add code that tests against contained pieces

	Dbg_Assert(m_cell_area.GetW() > 0 && m_cell_area.GetL() > 0 && m_cell_area.GetH() > 0);
	Dbg_MsgAssert(area.GetW() > 0 && area.GetL() > 0 && area.GetH() > 0, ("invalid dimensions (%d,%d,%d)", 
																		  area.GetW(), area.GetW(), area.GetL()));

	// do quick test
	
	if (m_cell_area.GetX() + m_cell_area.GetW() <= area.GetX())
		return false;
	if (m_cell_area.GetX() >= area.GetX() + area.GetW())
		return false;
	if (m_cell_area.GetY() + m_cell_area.GetH() <= area.GetY())
		return false;
	if (m_cell_area.GetY() >= area.GetY() + area.GetH())
		return false;
	if (m_cell_area.GetZ() + m_cell_area.GetL() <= area.GetZ())
		return false;
	if (m_cell_area.GetZ() >= area.GetZ() + area.GetL())
		return false;

	bool contains_risers = false;
   	if (excludeRisers)
	{
		for (uint i = 0; i < m_num_used_entries; i++)
		{
			SMetaDescriptor &desc = get_desc_at_index(i);
			if (desc.mRiser) contains_risers = true;
		}
	}
	// '|| if !excludeRisers' is implicit
	if (!contains_risers) return true;
	
	for (uint i = 0; i < m_num_used_entries; i++)
	{
		SMetaDescriptor &desc = get_desc_at_index(i);
		CClonedPiece *p_piece = GetContainedPiece(i)->CastToCClonedPiece();
		GridDims piece_area;
		p_piece->GetCellDims(&piece_area);
		piece_area[X] = m_cell_area.GetX() + desc.mX;
		piece_area[Y] = m_cell_area.GetY() + desc.mY;
		piece_area[Z] = m_cell_area.GetZ() + desc.mZ;
	
		if (!desc.mRiser &&
			piece_area.GetX() + piece_area.GetW() > area.GetX() &&
			piece_area.GetY() + piece_area.GetH() > area.GetY()	&&
			piece_area.GetZ() + piece_area.GetL() > area.GetZ() &&
			piece_area.GetX() < area.GetX() + area.GetW() &&
			piece_area.GetY() < area.GetY() + area.GetH() &&
			piece_area.GetZ() < area.GetZ() + area.GetL())
		{
			return true;
		}
	}
		
	return false;
}




/*
	See comments with original declaration.
	
	It is fine if pPiece has been rotated already.
*/
void CConcreteMetaPiece::add_piece(CPiece *pPiece, GridDims *pPos, Mth::ERot90 rot, bool isRiser)
{
	Dbg_Assert(pPiece);
	Dbg_MsgAssert(!(pPiece->GetFlags() & CPiece::mIN_WORLD), ("piece already in world"));
	
	CClonedPiece *p_cloned = pPiece->CastToCClonedPiece();
	#ifdef __NOPT_ASSERT__
	if (m_num_used_entries > 0)
	{
		if (get_desc_at_index(0).mpPiece->GetFlags() & CPiece::mSOFT_PIECE)
		{
			Dbg_MsgAssert(pPiece->GetFlags() & CPiece::mSOFT_PIECE, ("must be a soft piece"));
		}
		else
		{
			Dbg_MsgAssert(!(pPiece->GetFlags() & CPiece::mSOFT_PIECE), ("must be a hard piece"));
		}
	}
	#endif

	Dbg_MsgAssert(m_num_used_entries < m_total_entries, ("out of entries"));
	SMetaDescriptor &desc = get_desc_at_index(m_num_used_entries++);

	// Set position relative to northwest bottom of piece. For now.
	Mth::Vector piece_dims = p_cloned->GetDims();
	Mth::Vector desired_pos;
	desired_pos.Set(pPos->GetX() * CParkGenerator::CELL_WIDTH + piece_dims.GetX() / 2.0f,
						  pPos->GetY() * CParkGenerator::CELL_HEIGHT, // + piece_dims.GetY() / 2.0f,
						  pPos->GetZ() * CParkGenerator::CELL_LENGTH + piece_dims.GetZ() / 2.0f);
	p_cloned->SetDesiredPos(desired_pos, CClonedPiece::MARKER_ONLY);

	desc.mpPiece = p_cloned;
	desc.mX = pPos->GetX();
	desc.mY = pPos->GetY();
	desc.mZ = pPos->GetZ();
	desc.mRot = rot;
	desc.mRiser = isRiser ? 1 : 0;

	/*
	if (isRiser)
	{
		printf("hoogaboog 2 (%d,%d,%d), desc.mRiser is %d, desc.mRiser at 0x%x, index %d\n", 
			   desc.mX, desc.mY, desc.mZ, desc.mRiser, &desc.mRiser, m_num_used_entries-1);
	}
	*/
}




CAbstractMetaPiece::CAbstractMetaPiece()
{
	m_flags = EFlags(m_flags | mABSTRACT_META);
}




CPiece *CAbstractMetaPiece::GetContainedPiece(int index)
{
	if ((uint) index >= m_num_used_entries || index < 0)
		return NULL;

	CPiece *p_piece = CParkManager::sInstance()->GetGenerator()->GetMasterPiece(get_desc_at_index(index).mPieceName);
	Dbg_Assert(p_piece);
	return p_piece;
}




/*
	See comments with original declaration. It is fine if pPiece has been rotated already.
	
	Automatically changes stored cell WHL dimensions of metapiece to include new addition.
*/
void CAbstractMetaPiece::add_piece(CPiece *pPiece, GridDims *pPos, Mth::ERot90 rot, bool isRiser)
{
	Dbg_Assert(pPiece);
	CSourcePiece *p_source = NULL;
	if (pPiece->GetFlags() & CPiece::mSOURCE_PIECE)
		p_source = pPiece->CastToCSourcePiece();
	if (!p_source)
		Dbg_MsgAssert(0, ("not supported yet"));

	Dbg_MsgAssert(m_num_used_entries < m_total_entries, ("out of entries"));
	SMetaDescriptor &desc = get_desc_at_index(m_num_used_entries++);

	if (p_source)
	{
		desc.mX = pPos->GetX();
		desc.mY = pPos->GetY();
		desc.mZ = pPos->GetZ();
		desc.mRot = rot;
		desc.mRiser = (uint8) isRiser;
		desc.mPieceName = p_source->GetType();

		int highest_x = pPos->GetX() + pPos->GetW();
		int highest_y = pPos->GetY() + pPos->GetH();
		int highest_z = pPos->GetZ() + pPos->GetL();

		uint8 current_w = m_cell_area.GetW();
		uint8 current_h = m_cell_area.GetH();
		uint8 current_l = m_cell_area.GetL();
		
		if (highest_x > current_w)
			current_w = highest_x;
		if (highest_y > current_h)
			current_h = highest_y;
		if (highest_z > current_l)
			current_l = highest_z;

		m_cell_area.SetWHL(current_w, current_h, current_l);
		if (m_cell_area[H] < 1) m_cell_area[H] = 1;
	}
	
	/*
	if (isRiser)
	{
		printf("hoogaboog 1 (%d,%d,%d), index %d\n", desc.mX, desc.mY, desc.mZ, m_num_used_entries-1);
	}
	*/
}




void CAbstractMetaPiece::add_piece_dumb(CAbstractMetaPiece *pMeta, GridDims *pPos, Mth::ERot90 rot, bool isRiser)
{
	Dbg_MsgAssert(m_num_used_entries < m_total_entries, ("out of entries"));
	SMetaDescriptor &desc = get_desc_at_index(m_num_used_entries++);

	if (pMeta)
	{
		desc.mX = pPos->GetX();
		desc.mY = pPos->GetY();
		desc.mZ = pPos->GetZ();
		desc.mRot = rot;
		desc.mRiser = (uint8) isRiser;
		desc.mPieceName = pMeta->GetNameChecksum();

		int highest_x = pPos->GetX() + pPos->GetW();
		int highest_y = pPos->GetY() + pPos->GetH();
		int highest_z = pPos->GetZ() + pPos->GetL();

		uint8 current_w = m_cell_area.GetW();
		uint8 current_h = m_cell_area.GetH();
		uint8 current_l = m_cell_area.GetL();
		
		if (highest_x > current_w)
			current_w = highest_x;
		if (highest_y > current_h)
			current_h = highest_y;
		if (highest_z > current_l)
			current_l = highest_z;

		m_cell_area.SetWHL(current_w, current_h, current_l);
	}
	
	/*
	if (isRiser)
	{
		printf("hoogaboog 1 (%d,%d,%d), index %d\n", desc.mX, desc.mY, desc.mZ, m_num_used_entries-1);
	}
	*/
}




SMetaDescriptor CAbstractMetaPiece::get_desc_with_expansion(int index)
{
	#if 1
	int count = 0;
	for (uint j = 0; j < m_num_used_entries; j++)
	{
		SMetaDescriptor &outer_desc = get_desc_at_index(j);

		CAbstractMetaPiece *p_contained_meta = CParkManager::sInstance()->GetAbstractMeta(outer_desc.mPieceName);
		if (p_contained_meta->m_flags & CMetaPiece::mSINGULAR)
		{
			if (count == index)
				return outer_desc;
			count++;
		}
		else
		{
			for (uint i = 0; i < p_contained_meta->m_num_used_entries; i++)
			{
				SMetaDescriptor &inner_desc = p_contained_meta->get_desc_at_index(i);
				if (count == index)
				{
					SMetaDescriptor ret_desc = inner_desc;
					ret_desc.mPieceName = inner_desc.mPieceName;
					ret_desc.mRot = inner_desc.mRot + outer_desc.mRot;
					
					GridDims outer_dims = GetArea();

					GridDims inner_dims = p_contained_meta->GetArea();
					
					ret_desc.mX = outer_desc.mX;
					ret_desc.mZ = outer_desc.mZ;
					ret_desc.mY = outer_desc.mY + inner_desc.mY;
					
					switch(outer_desc.mRot)
					{
						case Mth::ROT_0:
							ret_desc.mX += inner_desc.mX;
							ret_desc.mZ += inner_desc.mZ;
							break;
						case Mth::ROT_90:
							ret_desc.mX += inner_desc.mZ;
							ret_desc.mZ += outer_dims.GetW() - inner_desc.mX - inner_dims.GetW();
							break;
						case Mth::ROT_180:
							ret_desc.mX += outer_dims.GetW() - inner_desc.mX - inner_dims.GetW();
							ret_desc.mZ += outer_dims.GetL() - inner_desc.mZ - inner_dims.GetL();
							break;
						case Mth::ROT_270:
							ret_desc.mX += outer_dims.GetL() - inner_desc.mZ - inner_dims.GetL();
							ret_desc.mZ += inner_desc.mX;
							break;
					}
					
					return ret_desc;
				}
				count++;
			}
		}
	}
	Dbg_MsgAssert(0, ("what the bloody 'ell?"));
	SMetaDescriptor dumb;
	return dumb;
	#else
	return get_desc_at_index(index);
	#endif
}




int CAbstractMetaPiece::count_descriptors_expanded()
{
	#if 1
	int count = 0;
	for (uint j = 0; j < m_num_used_entries; j++)
	{
		SMetaDescriptor &outer_desc = get_desc_at_index(j);

		CAbstractMetaPiece *p_contained_meta = CParkManager::sInstance()->GetAbstractMeta(outer_desc.mPieceName);
		Dbg_MsgAssert(p_contained_meta, ("couldn't find metapiece %s", Script::FindChecksumName(outer_desc.mPieceName)));
		if (p_contained_meta->m_flags & CMetaPiece::mSINGULAR)
		{
			count++;
		}
		else
		{
			for (uint i = 0; i < p_contained_meta->m_num_used_entries; i++)
			{
				count++;
			}
		}
	}
	return count;
	#else
	return m_num_used_entries;
	#endif
}




CMetaPiece *CMapListNode::GetMeta()
{
	Dbg_Assert(this);
	Dbg_MsgAssert(((uint32)this) != 0x03030303,("Bad CMapListNode pointer ! (0x03030303)"));
	Dbg_MsgAssert(((uint32)this) != 0x55555555,("Bad CMapListNode pointer ! (0x55555555)"));
	Dbg_Assert(mp_meta);
	return mp_meta;
}




void CMapListNode::DestroyList()
{
	CMapListNode *p_node = this;
	while (p_node)
	{
		CMapListNode *p_next_node = p_node->mp_next;
		delete p_node;
		p_node = p_next_node;
	}
}




void CMapListNode::VerifyIntegrity()
{
	CMapListNode *p_outer_node = this;
	while (p_outer_node)
	{
		CMapListNode *p_inner_node = p_outer_node->mp_next;
		while (p_inner_node)
		{
			Dbg_Assert(p_inner_node->GetMeta() != p_outer_node->GetMeta());
			p_inner_node = p_inner_node->mp_next;
		}
		p_outer_node = p_outer_node->mp_next;
	}
}




CMapListTemp::CMapListTemp(CMapListNode *p_list)
{
	mp_list = p_list;
}




CMapListTemp::~CMapListTemp()
{
	if (mp_list)
		mp_list->DestroyList();
}




CMapListNode *CMapListTemp::GetList()
{
	return mp_list;
}




void CMapListTemp::PrintContents()
{
	printf("contents of map list:\n");
	CMapListNode *p_node = mp_list;
	while(p_node)
	{
		printf("   address=%p area=", p_node->GetMeta());
		p_node->GetMeta()->GetArea().PrintContents();
		p_node = p_node->GetNext();
	}
}




CParkManager::CParkManager()
{
	mp_generator = new CParkGenerator();
	
	mp_concrete_metapiece_list = NULL;
	mp_abstract_metapiece_list = NULL;
	m_num_concrete_metapieces = 0;
	m_num_general_concrete_metapieces = 0;
	m_dma_piece_count = 0;
	m_num_abstract_metapieces = 0;

	m_floor_height_map = NULL;
	
	for (int i = 0; i < FLOOR_MAX_WIDTH; i++)
		for (int j = 0; j < FLOOR_MAX_LENGTH; j++)
			mp_bucket_list[i][j] = NULL;

	m_state_on = false;

	sp_instance = this;

	mp_compressed_map_buffer = new uint8[COMPRESSED_MAP_SIZE];
	m_compressed_map_flags = mNO_FLAGS;
	m_park_is_valid = false;

	mp_gap_manager = NULL;
	
	for (int i = 0; i < NUM_RISER_INFO_SLOTS; i++)
	{
		char name0[128];
		sprintf(name0, "floor_wall_block%d", i+1);
		m_riser_piece_checksum[i][0] = Script::GenerateCRC(name0);
		
		char name1[128];
		sprintf(name1, "wall_block%d", i+1);
		m_riser_piece_checksum[i][1] = Script::GenerateCRC(name1);
		
		char name2[128];
		sprintf(name2, "floor_block%d", i+1);
		m_riser_piece_checksum[i][2] = Script::GenerateCRC(name2);
	}

	m_theme = 0;
	
	// Mick: Create a compressed map buffer, equivalent to an empty park								
	fake_compressed_map_buffer();

	mp_column_slide_list = NULL;	
	
	mp_park_name[0]=0;
	
	#ifdef USE_BUILD_LIST
	m_build_list_size=0;
	mp_build_list_entry=NULL;
	#endif
}




CParkManager::~CParkManager()
{
	#ifdef USE_BUILD_LIST
	if (mp_build_list_entry)
	{
		#ifndef	__USE_EXTERNAL_BUFFER__
		Mem::Free(mp_build_list_entry);
		#endif
		mp_build_list_entry=NULL;
	}	
	#endif
	
	delete mp_generator;
	delete mp_compressed_map_buffer;

	sp_instance = NULL;
}




/*
	Boots up park editor.
	
	-Loads up files needed
	-Initializes buffers
	-Initializes CParkGenerator module
	-Calls Rebuild(), which:
		-Clears map
		-Generates riser geometry
	-Creates piece set database
*/
void CParkManager::Initialize()
{
	ParkEd("CParkManager::Initialize()");
	
	if (m_state_on)
		return;
	m_state_on = true;


	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	Mem::CPoolable<CMapListNode>::SCreatePool(16000, "CMapListNode");
	Mem::CPoolable<CMapListNode>::SPrintInfo();

	Mem::CPoolable<CClipboardEntry>::SCreatePool(MAX_CLIPBOARD_METAS, "CClipboardEntry");
	Mem::CPoolable<CClipboard>::SCreatePool(MAX_CLIPBOARDS, "CClipboard");
	Mem::Manager::sHandle().PopContext();

	mp_generator->InitializeMasterPieces(MAX_WIDTH, 16, MAX_LENGTH, GetTheme());
	mp_generator->ReadInRailInfo();

	create_abstract_metapieces();

	setup_road_mask();
	
	Dbg_MsgAssert(!m_floor_height_map, ("bad monkey!!!"));
	m_floor_height_map = new FloorHeightEntry*[FLOOR_MAX_WIDTH];
	for (int i = 0; i < FLOOR_MAX_WIDTH; i++)
	{
		m_floor_height_map[i] = new FloorHeightEntry[FLOOR_MAX_LENGTH];		
		for (int j = 0; j < FLOOR_MAX_LENGTH; j++)
		{
			m_floor_height_map[i][j].mMarkAsSlid = false;
			m_floor_height_map[i][j].mSlideAmount = 0;
		}
	}


	setup_default_dimensions();
		
	mp_gap_manager = new CGapManager(this);
	
	RebuildWholePark(false);

	Script::CArray *p_set_array = Script::GetArray("Ed_piece_sets", Script::ASSERT);
	m_total_sets = p_set_array->GetSize();
	Dbg_Assert(m_total_sets <= MAX_SETS);

	Script::CArray *p_piece_array = Script::GetArray("Ed_standard_metapieces", Script::ASSERT);
	int last_piece_index=p_piece_array->GetSize();
	int last_set_index=-1;
	
	for (int s = m_total_sets-1; s >=0 ; --s)
	{
		Script::CStruct *p_set = p_set_array->GetStructure(s);
		
		m_palette_set[s].mpName = NULL;
		p_set->GetText("name", &m_palette_set[s].mpName, Script::ASSERT);
		m_palette_set[s].mNameCrc = Script::GenerateCRC(m_palette_set[s].mpName);
		m_palette_set[s].mSelectedEntry = 0;
		
		m_palette_set[s].mTotalEntries=0;

		uint32 first_checksum;
		if (p_set->GetChecksum("first", &first_checksum))
		{
			int first_index=0;
			if (!GetAbstractMeta(first_checksum, &first_index))
			{
				Dbg_MsgAssert(0, ("couldn't find first-in-set entry"));
			}

			// make sure index in previous first-in_set_entry is less than this one
			if (last_set_index>=0)
			{
				Dbg_MsgAssert(first_index <
							  last_piece_index,
							  ("sets %s and %s are out of order in EdPieces2.q", 
							   m_palette_set[s].mpName,
							   m_palette_set[last_set_index].mpName));
			}
			else
			{
				Dbg_MsgAssert(first_index <
							  last_piece_index,
							  ("set %s has a bad first piece in EdPieces2.q", 
							   m_palette_set[s].mpName));
			}
			m_palette_set[s].mTotalEntries=last_piece_index-first_index;
			last_piece_index=first_index;
			last_set_index=s;
			
			// Now that we know the number of entries, create the set.
			int entry=first_index;
			for (int i=0; i<m_palette_set[s].mTotalEntries; ++i)
			{
				CAbstractMetaPiece *p_abstract = GetAbstractMetaByIndex(entry);
				
				Dbg_Assert(i < CPieceSet::MAX_ENTRIES);
				m_palette_set[s].mEntryTab[i].mNameCrc = p_abstract->GetNameChecksum();
				
				Script::CStruct *p_piece_entry = p_piece_array->GetStructure(entry);
				if (!p_piece_entry->GetLocalString("text_name", &m_palette_set[s].mEntryTab[i].mpName))
				{
					m_palette_set[s].mEntryTab[i].mpName = NULL;
				}
			
				++entry;
			}
		}

		m_palette_set[s].mIsClipboardSet=false;
		if (p_set->ContainsFlag(CRCD(0x45c6c158,"clipboard_set")))
		{
			m_palette_set[s].mIsClipboardSet=true;
			m_palette_set[s].mTotalEntries=MAX_CLIPBOARDS;
			
			const char *p_clipboard_title=Script::GetString(CRCD(0x4e677220,"ClipboardTitle"));
			
			for (int i=0; i<MAX_CLIPBOARDS; ++i)
			{
				char p_name[100];
				sprintf(p_name,"Clipboard%d",i);
				
				m_palette_set[s].mEntryTab[i].mpName=p_clipboard_title;
				m_palette_set[s].mEntryTab[i].mNameCrc=Script::GenerateCRC(p_name);
			}	
		}
		
		m_palette_set[s].mVisible = !p_set->ContainsFlag("hidden");
		#ifdef __PLAT_NGC__
		if (p_set->ContainsFlag("no_gamecube"))
			m_palette_set[s].mVisible = false;
		#endif
	}
}

float CParkManager::GetClipboardProportionUsed()
{
	float proportion_clipboard_entries=((float)Mem::CPoolable<CClipboardEntry>::SGetNumUsedItems())/Mem::CPoolable<CClipboardEntry>::SGetTotalItems();
	float proportion_clipboard_slots=((float)Mem::CPoolable<CClipboard>::SGetNumUsedItems())/Mem::CPoolable<CClipboard>::SGetTotalItems();

	if (proportion_clipboard_entries > proportion_clipboard_slots)
	{
		return proportion_clipboard_entries;
	}
	return proportion_clipboard_slots;
}
	
// This is just a public version of the private function
// needed so the parks can be saved from within the park editor
// This just updates the contents of the map buffer that will be used
// by the enxt two public functions
void CParkManager::WriteCompressedMapBuffer()
{
	if (m_state_on)
		write_compressed_map_buffer();	
}

uint8* CParkManager::GetCompressedMapBuffer(bool markSaved)
{
	/*
		The compressed map buffer is valid if:
		
		-The park manager is active AND (in sync with built park OR newer than built park) OR
		-The park manager is not active
	*/
	
	Dbg_MsgAssert((m_compressed_map_flags & mIS_VALID), ("compressed map buffer not valid")); 
	if (m_state_on)
	{
		Dbg_MsgAssert((m_compressed_map_flags & mIN_SYNC_WITH_PARK) || (m_compressed_map_flags & mIS_NEWER_THAN_PARK),
					  ("park has changed, compressed map buffer out of sync"));
	}

#ifdef __NOPT_ASSERT__
	CompressedMapHeader *p_header = (CompressedMapHeader *) mp_compressed_map_buffer;
	Dbg_Printf( "************ NUM METAS: %d\n", p_header->mNumMetas );
#endif		// __NOPT_ASSERT__

	if (markSaved)
		m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~mNOT_SAVED_LOCAL);
	
	return mp_compressed_map_buffer;
}




void	CParkManager::SetCompressedMapBuffer( uint8* src_buffer, bool markNotSaved )
{
	ParkEd("SetCompressedMapBuffer()");
	
	
	memcpy( mp_compressed_map_buffer, src_buffer, COMPRESSED_MAP_SIZE );
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags | mIS_VALID);
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags | mIS_NEWER_THAN_PARK);

	if (markNotSaved)
		m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags | mNOT_SAVED_LOCAL);

#ifdef __NOPT_ASSERT__
	CompressedMapHeader *p_header = (CompressedMapHeader *) mp_compressed_map_buffer;
	//uint32 checksum=Crc::GenerateCRCCaseSensitive((const char*)mp_compressed_map_buffer+4,COMPRESSED_MAP_SIZE-4);
	//Dbg_MsgAssert(checksum==p_header->mChecksum,("Park editor buffer checksum mismatch !"));
	
	Dbg_Printf( "************ NUM METAS: %d\n", p_header->mNumMetas );
#endif		// __NOPT_ASSERT__
}




bool CParkManager::IsMapSavedLocally()
{
	if ((!(m_compressed_map_flags & mIN_SYNC_WITH_PARK) && !(m_compressed_map_flags & mIS_NEWER_THAN_PARK)) || 
		(m_compressed_map_flags & mNOT_SAVED_LOCAL))
		return false;
	return true;
}


void CParkManager::WriteIntoStructure(Script::CStruct *p_struct)
{
	WriteCompressedMapBuffer();		// Ensure map buffer is correct
	uint32	*p_map_buffer = (uint32*) GetCompressedMapBuffer(true);
	int		size = COMPRESSED_MAP_SIZE ;

	// create the array
	Script::CArray *p_map=new Script::CArray;
	p_map->SetArrayType(size/4,ESYMBOLTYPE_INTEGER);
	// copy the park editor into it
	for (int i=0;i<size/4;i++)
	{
		p_map->SetInteger(i,p_map_buffer[i]);
	}
	
	// append it to p_struct
	p_struct->AddArrayPointer(CRCD(0x337c5289,"Park_editor_map"),p_map);

	// Now also add in any created goals, so that the goals do not have to saved out to a separate file.
	Script::CStruct *p_goals_struct=new Script::CStruct;
	
	Obj::CGoalEditorComponent *p_goal_editor=Obj::GetGoalEditor();
	Dbg_MsgAssert(p_goal_editor,("No goal editor component ???"));
	p_goal_editor->WriteIntoStructure(CRCD(0xe8b4b836,"Load_Sk5Ed"),p_goals_struct);
	
	
	p_struct->AddStructurePointer(CRCD(0xd8eb825e,"Park_editor_goals"),p_goals_struct);

	// Add in the created rails.			
	Obj::GetRailEditor()->WriteIntoStructure(p_struct);

	p_struct->AddInteger(CRCD(0xb7e39b53,"MaxPlayers"),GetGenerator()->GetMaxPlayers());
}

#ifdef __PLAT_NGC__
void CParkManager::ReadFromStructure(Script::CStruct *p_struct, bool do_post_mem_card_load, bool preMadePark)
#else
void CParkManager::ReadFromStructure(Script::CStruct *p_struct, bool do_post_mem_card_load)
#endif
{
	Obj::CGoalEditorComponent *p_goal_editor=Obj::GetGoalEditor();
	Dbg_MsgAssert(p_goal_editor,("No goal editor component ???"));
	
	p_goal_editor->ClearOnlyParkGoals();
	
	// Read in any created goals.
	Script::CStruct *p_goals_struct=NULL;
	if (p_struct->GetStructure(CRCD(0xd8eb825e,"Park_editor_goals"),&p_goals_struct))
	{
		p_goal_editor->ReadFromStructure(p_goals_struct,Obj::CGoalEditorComponent::LOADING_PARK_GOALS);
	}

	// Read in the created rails
	Spt::SingletonPtr<Ed::CParkEditor> p_park_ed;
	if (!p_park_ed->EditingCustomPark())
	{
		// Do not call UpdateSuperSectors after deleting any existing rails if playing a park,
		// otherwise the entire park will disappear from under the skater due to all the sectors
		// having been flagged for deletion.
		Obj::CRailEditorComponent::sUpdateSuperSectorsAfterDeletingRailSectors=false;
	}	
	Obj::GetRailEditor()->ReadFromStructure(p_struct);
	Obj::CRailEditorComponent::sUpdateSuperSectorsAfterDeletingRailSectors=true;
	
	int max_players=2;
	p_struct->GetInteger(CRCD(0xb7e39b53,"MaxPlayers"),&max_players);
	Ed::CParkManager::Instance()->GetGenerator()->SetMaxPlayers(max_players);
				
	int old_theme = GetTheme();
	WriteCompressedMapBuffer();
	uint32	*p_map_buffer = (uint32*) GetCompressedMapBuffer();
	int		size = Ed::CParkManager::COMPRESSED_MAP_SIZE ;
	// get the array
	Script::CArray *p_map=NULL;
	if (p_struct->GetArray("Park_editor_map",&p_map,true))
	{
		// copy the park editor map from structure to buffer
		for (int i=0;i<size/4;i++)
		{
			#ifdef __PLAT_NGC__
			if (preMadePark)
			{
				uint32 v=p_map->GetInteger(i);
				p_map_buffer[i] = ((v>>24) | (((v>>16)&0xff)<<8) | (((v>>8)&0xff)<<16) | ((v&0xff)<<24));
			}	
			else
			{
				p_map_buffer[i] = p_map->GetInteger(i);
			}		
			#else
			p_map_buffer[i] = p_map->GetInteger(i);
			#endif
		}

		#ifdef __PLAT_NGC__
		if (preMadePark)
		{
			// If loading a pre-made park, it will need the map buffer endianness swapped since
			// it was saved from the PS2.
			SwapMapBufferEndianness();
		}	
		#endif
		
		if (do_post_mem_card_load)
		{
			Spt::SingletonPtr<Ed::CParkEditor> p_park_ed;
			p_park_ed->PostMemoryCardLoad((uint8*) p_map_buffer, old_theme);
		}	
	}
}

int CParkManager::GetTheme()
{
	if (m_compressed_map_flags & mIS_VALID)
	{
		int theme = (((CompressedMapHeader *) mp_compressed_map_buffer)->mTheme);
		Dbg_MsgAssert(theme >= 0 && theme <= MAX_THEMES, ("nonsense theme %d", theme));
		return theme;
	}

	return 0;
}




void CParkManager::SetTheme(int theme)
{
	Dbg_MsgAssert(theme >= 0 && theme < MAX_THEMES , ("nonsense theme %d", theme));
	m_theme = theme;
	write_compressed_map_buffer();
}




const char *CParkManager::GetParkName()
{
	if (mp_compressed_map_buffer && ((CompressedMapHeader *) mp_compressed_map_buffer)->mVersion == (VERSION))
		return ((CompressedMapHeader *) mp_compressed_map_buffer)->mParkName;
	else
		return NULL;
}

uint32 CParkManager::GetParkChecksum()
{
	if (mp_compressed_map_buffer && ((CompressedMapHeader *) mp_compressed_map_buffer)->mVersion == (VERSION))
		return ((CompressedMapHeader *) mp_compressed_map_buffer)->mChecksum;
	else
		return 0;
}

int	CParkManager::GetCompressedParkWidth()
{
	if (mp_compressed_map_buffer && ((CompressedMapHeader *) mp_compressed_map_buffer)->mVersion == (VERSION))
		return ((CompressedMapHeader *) mp_compressed_map_buffer)->mW;
	else
		return 0;
}

int	CParkManager::GetCompressedParkLength()
{
	if (mp_compressed_map_buffer && ((CompressedMapHeader *) mp_compressed_map_buffer)->mVersion == (VERSION))
		return ((CompressedMapHeader *) mp_compressed_map_buffer)->mL;
	else
		return 0;
}


void CParkManager::SetParkName(const char *pName)
{
	if (!mp_compressed_map_buffer || ((CompressedMapHeader *) mp_compressed_map_buffer)->mVersion != (VERSION)) return;
	Dbg_Assert(pName);
	Dbg_MsgAssert(strlen(pName) < 64, ("park name too long"));
	strcpy(((CompressedMapHeader *) mp_compressed_map_buffer)->mParkName, pName);
	strcpy(mp_park_name, pName);
}




void CParkManager::AccessDisk(bool save, int fileSlot)
{
	char fullname[64];
	sprintf(fullname, "CustomParks\\custom%d.prk", fileSlot);

	if (save)
	{
//		Ryan("I am saving park %s\n", fullname);
//		printf("I am saving park %s\n", fullname);
		
		Script::CStruct *p_struct=new Script::CStruct;
		WriteIntoStructure(p_struct);
		uint32 size=Script::CalculateBufferSize(p_struct);
		uint8 *p_buffer=(uint8*)Mem::Malloc(size);
		Script::WriteToBuffer(p_struct,p_buffer,size);
		
		void *fp = File::Open(fullname, "wb");
		Dbg_MsgAssert(fp, ("failed to open file %s for write", fullname));
		File::Write(p_buffer, size, 1, fp);
		File::Close(fp);
		
		Mem::Free(p_buffer);
		delete p_struct;
	}
	else
	{
//		Ryan("I am loading park %s\n", fullname);
//		printf("I am loading park %s\n", fullname);
		void *fp = File::Open(fullname, "rb");
		Dbg_MsgAssert(fp, ("failed to open file %s for read", fullname));
		
		int size=File::GetFileSize(fp);

		uint8 *p_buffer=(uint8*)Mem::Malloc(size);
		
		File::Read(p_buffer, size, 1, fp);
		File::Close(fp);
		
		Script::CStruct *p_struct=new Script::CStruct;
		Script::ReadFromBuffer(p_struct,p_buffer);
		Mem::Free(p_buffer);

		#ifdef __PLAT_NGC__
		ReadFromStructure(p_struct,false,true);
		#else
		ReadFromStructure(p_struct,false);
		#endif
		
		delete p_struct;
				
		m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags | (mIS_NEWER_THAN_PARK + mIS_VALID));
		m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~mIN_SYNC_WITH_PARK);
		m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~mNOT_SAVED_LOCAL);
	}	
}




/*
	Destroys everything, concrete metas, abstract metas, floor map.
*/
// K: Added type so that DESTROY_ONLY_PIECES can be passed when this function is used to clean
// up the park editor heap for deletion when playing a park.
void CParkManager::Destroy(CParkGenerator::EDestroyType type)
{
	ParkEd("CParkManager::Destroy(), m_compressed_map_flags=%08x", m_compressed_map_flags);
	
	if (!m_state_on)
		return;
	
	if (!(m_compressed_map_flags & mIN_SYNC_WITH_PARK) && !(m_compressed_map_flags & mIS_NEWER_THAN_PARK))
	{
		write_compressed_map_buffer();
	}
	
	Dbg_Assert(mp_gap_manager);
	mp_gap_manager->RemoveAllGaps();
	
	// remove all abstract metapieces
	CMapListNode *p_node = NULL;
	while((p_node = mp_abstract_metapiece_list))
	{
		CMetaPiece *p_meta = p_node->GetMeta();
		remove_metapiece_from_node_list(p_meta, &mp_abstract_metapiece_list);
		delete p_meta;
	}
	m_num_abstract_metapieces = 0;
	
	destroy_concrete_metapieces(type);
	
	mp_generator->UnloadMasterPieces();
	mp_generator->DestroyRailInfo();   		// info from node array (mostly rails)
	
	Dbg_MsgAssert(m_floor_height_map, ("bad bad monkey!!!"));
	for (int i = 0; i < FLOOR_MAX_WIDTH; i++)
		delete [] m_floor_height_map[i];
	delete [] m_floor_height_map;
	m_floor_height_map = NULL;
	
	m_state_on = false;
	// since park is destroyed, buffer is out of sync and newer than park
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~mIN_SYNC_WITH_PARK);
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags | mIS_NEWER_THAN_PARK);
	m_park_is_valid = false;
	
	Dbg_Assert(mp_gap_manager);
	delete mp_gap_manager;
	mp_gap_manager = NULL;
	
	if (mp_column_slide_list)
		mp_column_slide_list->DestroyList();
	mp_column_slide_list = NULL;
	
	Mem::CPoolable<CClipboard>::SRemovePool();	
	Mem::CPoolable<CClipboardEntry>::SRemovePool();	
	Mem::CPoolable<CMapListNode>::SRemovePool();
}


void CParkManager::RebuildWholePark(bool clearPark)
{
	ParkEd("CParkManager::RebuildWholePark() 0x%x", m_compressed_map_flags);

	setup_lighting();  // ensure light arrays are correct for current theme
	
	Dbg_Assert(m_state_on);
	
	if (!(m_compressed_map_flags & mIN_SYNC_WITH_PARK) && 
		!(m_compressed_map_flags & mIS_NEWER_THAN_PARK) && 
		m_park_is_valid)
	{
		//Ryan("writing compressed map buffer\n");
		write_compressed_map_buffer();
	}
	
	Dbg_Printf( "************* Rebuilding whole park : %d\n", (int) clearPark );
	destroy_concrete_metapieces(CParkGenerator::DESTROY_PIECES_AND_SECTORS);
	mp_generator->GenerateCollisionInfo(false);

	if (clearPark || !(m_compressed_map_flags & mIS_VALID))
	{
		//Ryan("clearing park\n");
		Dbg_MsgAssert(m_floor_height_map, ("bad bad monkey!!!"));
		
		for (int j = 0; j < FLOOR_MAX_LENGTH; j++)
		{
			for (int i = 0; i < FLOOR_MAX_WIDTH; i++)
			{
				int x = i;
				int z = j;
				m_floor_height_map[x][z].mHeight = 0; //Mth::Rnd(4);
				m_floor_height_map[x][z].mEnclosedFloor = 0;
				m_floor_height_map[x][z].mMarkAsSlid = false;
				m_floor_height_map[i][j].mSlideAmount = 0; 
			}
		}
		
		write_compressed_map_buffer();
		SetParkName("unnamed park");
		m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~mNOT_SAVED_LOCAL);
		clearPark = true;
	}	
	else
	{
		// use compressed map
		read_from_compressed_map_buffer();
	}
	
	RebuildInnerShell(m_park_near_bounds);
	generate_floor_pieces_from_height_map(clearPark, false);	
	
	mp_generator->GenerateCollisionInfo();
//	mp_generator->GenerateNodeInfo(mp_concrete_metapiece_list);

	m_park_is_valid = true;

	if (clearPark)
	{
		m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags | mIN_SYNC_WITH_PARK);
	}
	
	// K: Now that the park geometry has been built and collision info generated, 
	// update the created rails.
	Obj::GetRailEditor()->RefreshGeometry();
}




/*
	-Might clear map
	-Generates riser geometry (from height map)

	The only place we call this function with firstTestMemory true is from CCursor::ChangeFloorHeight()
*/
bool CParkManager::RebuildFloor(bool firstTestMemory)
{
	ParkEd("CParkManager::RebuildFloor()");
	
	Dbg_Assert(m_state_on);
	
	//Dbg_Printf( "************* Rebuilding floor\n" );

	if (firstTestMemory)
	{
		if (!generate_floor_pieces_from_height_map(false, true))
			return false;
	}

	generate_floor_pieces_from_height_map(false, false);
	
	//CConcreteMetaPiece *p_out = CreateConcreteMeta(GetAbstractMeta(Script::GenerateCRC("gloopy")));
	//GridDims dims(m_park_near_bounds.GetX(), 0, m_park_near_bounds.GetZ());
	//AddMetaPieceToPark(dims, p_out);
	
	mp_generator->GenerateCollisionInfo();

	return true;
	
}


void CParkManager::RebuildNodeArray()
{
	ParkEd("CParkManager::RebuildNodeArray()");


	// clear any pre-exisiting automatic restarts
	// so we have slots free to add new ones	
	mp_generator->ClearAutomaticRestarts();

 
	// add default restarts
	// (they won't be added if player-placed ones already exist)
	// there are 18 points
	// 1p, 2p, 4 flags, 6 horse, 6 KOTH
	for (int p = 0; p < 18; p++)
	{
		CParkGenerator::RestartType restart_type = CParkGenerator::vONE_PLAYER;
		
		GridDims restart_area;
		restart_area = GetParkNearBounds();		 // returns the rectangle enclosed by the fence
		restart_area.SetXYZ(restart_area.GetX()+restart_area.GetW()/2, 0, restart_area.GetZ()+restart_area.GetL()/2);	// sets xyz to the midpoint
		restart_area.SetWHL(1,1,1);		// needed for when we get the floor height
						

		bool	find_ground = true;			// most restarts need to find the ground, except copied horse points
						
		int need = 1;		// Most restarts only need one.  Horse/Multi being the current exception						
		if (p == 1)
		{
			restart_type = CParkGenerator::vMULTIPLAYER;
			restart_area.SetXYZ(restart_area.GetX()+1, 0, restart_area.GetZ());	
		}

		if (p == 2)
		{
			restart_type = CParkGenerator::vRED_FLAG;
			restart_area.SetXYZ(restart_area.GetX()-2, 0, restart_area.GetZ()+2);	
		}

		if (p == 3)
		{
			restart_type = CParkGenerator::vGREEN_FLAG;
			restart_area.SetXYZ(restart_area.GetX()+0, 0, restart_area.GetZ()+2);	
		}

		if (p == 4)
		{
			restart_type = CParkGenerator::vBLUE_FLAG;
			restart_area.SetXYZ(restart_area.GetX()+2, 0, restart_area.GetZ()+2);	
		}

		if (p == 5)
		{
			restart_type = CParkGenerator::vYELLOW_FLAG;
			restart_area.SetXYZ(restart_area.GetX()+4, 0, restart_area.GetZ()+2);	
		}

		// all the dummy horse/multi restart points go in a line at Z-2		
		if (p >= 6 && p <=11)				 	// 6,7,8,9,10,11
		{
			need = 6;			
			restart_type = CParkGenerator::vHORSE;
			
			#if 0
			// Code to make horse points be evenly distributed between 1p/2p nodes
			// but it's not usable, as the players will start at the same point
			// either all multi points need to be different, of the code needs
			// to check in 2P (and multi) for same node positions
			// either are too risky to implement right now 			
			static GridDims horse_points[6];    // needs to be static, so I can include it in this conditional
			GridDims  dims;
			if (p==6)
			{
				// about to try the first horse point
				// so fill the array with appropriate values
				int num_actual_horse = mp_generator->NumRestartsOfType(restart_type);
//				printf ("There are %d actual horse points\n",num_actual_horse);
				if (num_actual_horse == 0)
				{
//					printf ("No horse points, using 1p/2p\n");
					// not even got one, so fill the array with 1p,2p,1p,2p,1p,2p
					for (int i = 0;i<6;i+=2)
					{
						dims = mp_generator->GetRestartDims(CParkGenerator::vONE_PLAYER,0);
						horse_points[i] = restart_area;
						horse_points[i].SetXYZ(dims.GetX(),0,dims.GetZ());
						dims = mp_generator->GetRestartDims(CParkGenerator::vMULTIPLAYER,0);
						horse_points[i+1] = restart_area;
						horse_points[i+1].SetXYZ(dims.GetX(),0,dims.GetZ());
					}					 
				}
				else
				{
					// we have one or more real horse points, so find out where they are
					// and copy those values for any automatic horse points, if needed
					for (int i = 0;i<6;i++)
					{
						if (i < num_actual_horse)
						{
							// copy the n initial values into the array
							horse_points[i] = mp_generator->GetRestartDims(CParkGenerator::vHORSE,i);
						}
						else
						{
							// and duplicate these through the rest of the array
							//  so we will get an even distribution
							//  n = 1  gives [ 1 1 1 1 1 1 ]
							//  n = 2  gives [ 1 2 1 2 1 2 ]
							//  n = 3  gives [ 1 2 3 1 2 3 ]
							//  n = 4  gives [ 1 2 3 4 1 2 ]
							//  n = 5  gives [ 1 2 3 4 5 1 ]
							//  n = 6  gives [ 1 2 3 4 5 6 ]
							horse_points[i] = horse_points[i % num_actual_horse];
						}
//						printf ("Horse %d: (%d,%d,%d)\n",horse_points[i].GetX(),horse_points[i].GetY(),horse_points[i].GetX());
					}					 					
				}
			}
			
			// copy the appropiate value from the array
			restart_area = horse_points[p-6];
			find_ground = false;			// Don't find the ground when copying existing restarts, as we will be raised up by 1p/2p/horse pieces
			//printf ("Using Restart Area(%d,%d,%d)\n",restart_area.GetX(), restart_area.GetY(), restart_area.GetZ());

															 
			#else			
			restart_area.SetXYZ(restart_area.GetX()-3 + (p-6), 0, restart_area.GetZ()-2);	
			//printf ("Using Restart Area(%d,%d,%d)\n",restart_area.GetX(), restart_area.GetY(), restart_area.GetZ());
			#endif
		
		}
		
		bool	auto_copy = false;
		
		if ( p >= 12 && p <= 17 )  					// 12,13,14,15,16,17
		{
			auto_copy = true;						// copy any existing restart point position, if there is one
			need = 6;
			restart_type = CParkGenerator::vKING_OF_HILL;
			restart_area.SetXYZ(restart_area.GetX()-3 + (p-12), 0, restart_area.GetZ()-3);	 
		}

		if (mp_generator->NotEnoughRestartsOfType(restart_type, need))
		{
			if (find_ground)
			{
				restart_area.SetXYZ(restart_area.GetX(),GetFloorHeight(restart_area),restart_area.GetZ());
				CMapListTemp meta_list = GetMetaPiecesAt(restart_area);
				if (!meta_list.IsEmpty())
				{
					restart_area[Y] = restart_area.GetY() + meta_list.GetList()->GetConcreteMeta()->GetArea().GetH();
				}
			}

			Mth::Vector restart_pos = GridCoordinatesToWorld(restart_area);
			restart_pos[X] = restart_pos[X] + CParkGenerator::CELL_WIDTH / 2.0f;
			restart_pos[Z] = restart_pos[Z] + CParkGenerator::CELL_LENGTH / 2.0f;
			
			// Fix to TT12221, where the flags would be floating if over a pool.
			CFeeler feeler;
			feeler.SetStart(restart_pos+Mth::Vector(0.0f, 10000.0f, 0.0f));
			feeler.SetEnd(restart_pos+Mth::Vector(0.0f, -1000.0f, 0.0f));
			if (feeler.GetCollision())
			{
				restart_pos[Y]=feeler.GetPoint().GetY();
			}	

//			printf("restart point %d at: cell=(%d,%d,%d) world=(%.4f,%.4f,%.4f)\n", p, 
//				 restart_area.GetX(), restart_area.GetY(), restart_area.GetZ(),
//				 restart_pos[X], restart_pos[Y], restart_pos[Z]);

			mp_generator->AddRestart(restart_pos, 0, restart_area, restart_type, true, auto_copy);
		}
	} // end for p

	mp_generator->GenerateNodeInfo(mp_concrete_metapiece_list);	
}


uint32 CParkManager::GetShellSceneID()
{
	char p_shell_name[40];
	if (GetTheme())
	{
		sprintf(p_shell_name,"sk5ed%d_shell",GetTheme()+1);
	}
	else
	{
		strcpy(p_shell_name,"sk5ed_shell");
	}
	return Script::GenerateCRC(p_shell_name);
}

void CParkManager::RebuildInnerShell(GridDims newNearBounds)
{
	ParkEd("CParkManager::RebuildInnerShell()");
	
	Dbg_Printf( "************* Rebuilding inner shell\n" );

	#if 1
	ParkEd("old shell bounds: (%d,%d,%d)-(%d,%d,%d)\n", 
		 m_park_near_bounds.GetX(), m_park_near_bounds.GetY(), m_park_near_bounds.GetZ(),
		 m_park_near_bounds.GetW(), m_park_near_bounds.GetH(), m_park_near_bounds.GetL());
	#endif
	m_park_near_bounds = newNearBounds;
	#if 1
	ParkEd("new shell bounds: (%d,%d,%d)-(%d,%d,%d)\n", 
		 newNearBounds.GetX(), newNearBounds.GetY(), newNearBounds.GetZ(),
		 newNearBounds.GetW(), newNearBounds.GetH(), newNearBounds.GetL());
	#endif
	
	// west
	m_road_mask_tab[0][X] = m_park_near_bounds.GetX() - 1;
	m_road_mask_tab[0][Z] = m_park_near_bounds.GetZ() - 1;
	m_road_mask_tab[0][W] = 1;
	m_road_mask_tab[0][L] = m_park_near_bounds.GetL() + 2;
	
	// east
	m_road_mask_tab[1][X] = m_park_near_bounds.GetX() + m_park_near_bounds.GetW();
	m_road_mask_tab[1][Z] = m_park_near_bounds.GetZ() - 1;
	m_road_mask_tab[1][W] = 1;
	m_road_mask_tab[1][L] = m_park_near_bounds.GetL() + 2;
	
	// north
	m_road_mask_tab[2][X] = m_park_near_bounds.GetX() - 1;
	m_road_mask_tab[2][Z] = m_park_near_bounds.GetZ() - 1;
	m_road_mask_tab[2][W] = m_park_near_bounds.GetW() + 2;
	m_road_mask_tab[2][L] = 1;
	
	// south
	m_road_mask_tab[3][X] = m_park_near_bounds.GetX() - 1;
	m_road_mask_tab[3][Z] = m_park_near_bounds.GetZ() + m_park_near_bounds.GetL();
	m_road_mask_tab[3][W] = m_park_near_bounds.GetW() + 2;
	m_road_mask_tab[3][L] = 1;
	
	//#if DEBUG_THIS_DAMN_THING
	//int destroy_type2_count = 0;
	//#endif
		
	// destroy all existing inner shell pieces
	// also: kill everything that's outside the near bounds
	CMapListNode *p_node = mp_concrete_metapiece_list;
	while(p_node)
	{
		CMapListNode *p_next_node = p_node->GetNext();
		GridDims meta_area = p_node->GetConcreteMeta()->GetArea();
		
		if (p_node->GetConcreteMeta()->IsInnerShell())
		{
			DestroyConcreteMeta(p_node->GetConcreteMeta(), mDONT_DESTROY_PIECES_ABOVE);
		}
		#if 1
		else if ( !p_node->GetConcreteMeta()->IsRiser() &&  
			p_node->GetConcreteMeta()->IsInPark() &&
			(meta_area.GetX() < m_park_near_bounds.GetX() ||
			meta_area.GetZ() < m_park_near_bounds.GetZ() ||
			meta_area.GetX() + meta_area.GetW() > m_park_near_bounds.GetX() + m_park_near_bounds.GetW() ||
			meta_area.GetZ() + meta_area.GetL() > m_park_near_bounds.GetZ() + m_park_near_bounds.GetL()))
		{
			DestroyConcreteMeta(p_node->GetConcreteMeta(), mDONT_DESTROY_PIECES_ABOVE);
			//#if DEBUG_THIS_DAMN_THING
			//destroy_type2_count++;
			//#endif
		}
		#endif

		p_node = p_next_node;
	}
	
	uint32 fence_meta_name_crc = Script::GenerateCRC("Sk4Ed_Fence_20x20");
	CAbstractMetaPiece *p_abstract_fence_meta = GetAbstractMeta(fence_meta_name_crc);
	
	for (int x = m_park_near_bounds.GetX(); x < m_park_near_bounds.GetX() + m_park_near_bounds.GetW(); x += 2)
	{
		GridDims road_test_area;
		road_test_area.SetWHL(2, 1, 1);

		// north
		GridDims pos(x, 0, m_park_near_bounds.GetZ() - 1);
		road_test_area.SetXYZ(pos.GetX(), 0, pos.GetZ() + 1);
		if (!IsOccupiedByRoad(road_test_area))
		{
			CConcreteMetaPiece *p_meta = CreateConcreteMeta(p_abstract_fence_meta);
			p_meta->SetRot(Mth::ROT_0);
			p_meta->MarkAsInnerShell();
			AddMetaPieceToPark(pos, p_meta);
		}
	
		// south
		pos.SetXYZ(x, 0, m_park_near_bounds.GetZ() + m_park_near_bounds.GetL());
		road_test_area.SetXYZ(pos.GetX(), 0, pos.GetZ() - 1);
		if (!IsOccupiedByRoad(road_test_area))
		{
			CConcreteMetaPiece *p_meta = CreateConcreteMeta(p_abstract_fence_meta);
			p_meta->SetRot(Mth::ROT_180);
			p_meta->MarkAsInnerShell();
			AddMetaPieceToPark(pos, p_meta);
		}
	}

	for (int z = m_park_near_bounds.GetZ(); z < m_park_near_bounds.GetZ() + m_park_near_bounds.GetL(); z += 2)
	{
		GridDims road_test_area;
		road_test_area.SetWHL(1, 1, 2);

		// west
		GridDims pos(m_park_near_bounds.GetX() - 1, 0, z);
		road_test_area.SetXYZ(pos.GetX() + 1, 0, pos.GetZ());
		if (!IsOccupiedByRoad(road_test_area))
		{
			CConcreteMetaPiece *p_meta = CreateConcreteMeta(p_abstract_fence_meta);
			p_meta->SetRot(Mth::ROT_90);
			p_meta->MarkAsInnerShell();
			AddMetaPieceToPark(pos, p_meta);
		}
	
		// east
		pos.SetXYZ(m_park_near_bounds.GetX() + m_park_near_bounds.GetW(), 0, z);
		road_test_area.SetXYZ(pos.GetX() - 1, 0, pos.GetZ());
		if (!IsOccupiedByRoad(road_test_area))
		{
			CConcreteMetaPiece *p_meta = CreateConcreteMeta(p_abstract_fence_meta);
			p_meta->SetRot(Mth::ROT_270);
			p_meta->MarkAsInnerShell();
			AddMetaPieceToPark(pos, p_meta);
		}
	}

	//ParkEd("destroyed %d non-riser pieces\n", destroy_type2_count); 

	for (int x = 0; x < MAX_WIDTH; x++)
	{
		for (int z = 0; z < MAX_LENGTH; z++)
		{
			if (x < m_park_near_bounds.GetX() ||
				z < m_park_near_bounds.GetZ() ||
				x >= m_park_near_bounds.GetX() + m_park_near_bounds.GetW() ||
				z >= m_park_near_bounds.GetZ() + m_park_near_bounds.GetL())
			{
				m_floor_height_map[x][z].mHeight = 0;
			}
		}
	}

	mp_generator->RemoveOuterShellPieces(GetTheme());
	mp_generator->GenerateCollisionInfo();

//	mp_generator->GenerateNodeInfo(mp_concrete_metapiece_list);
	ParkEd("end RebuildInnerShell()\n");
}




/*
	Retrieves the abstract meta with the specified name checksum. Returns its index.
*/
CAbstractMetaPiece *CParkManager::GetAbstractMeta(uint32 type, int *pRetIndex)
{
	Dbg_Assert(m_state_on);
	
	CMapListNode *p_node = mp_abstract_metapiece_list;
	int index = m_num_abstract_metapieces-1;
	while(p_node)
	{
		CAbstractMetaPiece *p_meta = p_node->GetAbstractMeta();
		Dbg_Assert(p_meta);
		if (p_meta->m_name_checksum == type)
		{
			if (pRetIndex)
				*pRetIndex = index;
			return p_meta;
		}

		index--;
		p_node = p_node->GetNext();
	}

	return NULL;
}




/*
	Retrieves the abstract meta with the specified index.
*/
CAbstractMetaPiece *CParkManager::GetAbstractMetaByIndex(int index)
{
	Dbg_Assert(m_state_on);
	
	CMapListNode *p_node = mp_abstract_metapiece_list;
	int i = m_num_abstract_metapieces-1;
	while(p_node)
	{
		CAbstractMetaPiece *p_meta = p_node->GetAbstractMeta();
		Dbg_Assert(p_meta);
		if (i == index)
			return p_meta;

		i--;
		p_node = p_node->GetNext();
	}

	Dbg_MsgAssert(0, ("index %d out of range", i));
	return NULL;	
}




/*
	Creates a concrete copy of the specified abstract meta.
*/
CConcreteMetaPiece *CParkManager::CreateConcreteMeta(CAbstractMetaPiece *pSource, bool isSoft)
{
	Dbg_Assert(m_state_on);
	Dbg_Assert(pSource);
	
	Mem::Manager::sHandle().PushContext(mp_generator->GetParkEditorHeap());
	CConcreteMetaPiece *p_meta = new CConcreteMetaPiece();
	Mem::Manager::sHandle().PopContext();
	
	int num_entries = pSource->count_descriptors_expanded();
	p_meta->initialize_desc_table(num_entries);
	p_meta->m_name_checksum = pSource->m_name_checksum;
	
	CPiece::EFlags clone_flags = (isSoft) ? CPiece::mSOFT_PIECE : CPiece::mNO_FLAGS;
	
	if (pSource->IsRiser())
	{
		p_meta->set_flag(CMetaPiece::mIS_RISER);
		clone_flags	= CPiece::EFlags(clone_flags | CPiece::mFLOOR);
	}
	if (pSource->NoRails())
	{
		p_meta->set_flag(CMetaPiece::mNO_RAILS);
		clone_flags	= CPiece::EFlags(clone_flags | CPiece::mNO_RAILS);
	}
	if (pSource->m_flags & CMetaPiece::mCONTAINS_RISERS)
	{
		p_meta->set_flag(CMetaPiece::mCONTAINS_RISERS);
	}

	// pick through source piece, for each descriptor entry, make new cloned piece
	for (int i = 0; i < num_entries; i++)
	{
		SMetaDescriptor desc = pSource->get_desc_with_expansion(i);
		
		CClonedPiece *p_piece = 
			mp_generator->ClonePiece(CParkManager::sInstance()->GetGenerator()->GetMasterPiece(desc.mPieceName), clone_flags);
		p_piece->SetDesiredRot(Mth::ERot90(desc.mRot), CClonedPiece::MARKER_ONLY);
		GridDims dims(desc.mX, desc.mY, desc.mZ);
		p_meta->add_piece(p_piece, &dims, Mth::ERot90(desc.mRot), (desc.mRiser != 0));

		#if 0
		// Ryan: this call is unnecessary
		if (!isSoft)
		{
			mp_generator->CalculateLighting(p_piece);
		}
		#endif

		#if 0
		if (pSource->GetNameChecksum() == Script::GenerateCRC("Low_Short_Deck"))
		{
			printf("   rotation: %d\n", desc.mRot);
		}
		#endif
	}

	p_meta->lock();
	   
	#if 1
	#ifdef __NOPT_ASSERT__
	GridDims test_area = p_meta->GetArea();
	Dbg_MsgAssert(test_area.GetW() > 0 && test_area.GetH() > 0 && test_area.GetL() > 0, 
				  ("metapiece %s has invalid dimensions (%d,%d,%d)", Script::FindChecksumName(p_meta->GetNameChecksum()),
				   test_area.GetW(), test_area.GetH(), test_area.GetL()));
	
	#if 0
	if (1 || p_meta->GetNameChecksum() == Script::GenerateCRC("Sk4Ed_Fence_10x10"))
		printf("OY! metapiece %s has dimensions (%d,%d,%d)\n", Script::FindChecksumName(p_meta->GetNameChecksum()),
				   test_area.GetW(), test_area.GetH(), test_area.GetL());
	#endif

	#endif
	#endif
	
	Mem::Manager::sHandle().PushContext(mp_generator->GetParkEditorHeap());
	CMapListNode *p_node = new CMapListNode();
	Mem::Manager::sHandle().PopContext();
	p_node->mp_meta = p_meta;

	#if 0	
	p_node->mp_next = mp_concrete_metapiece_list;
	mp_concrete_metapiece_list = p_node;
	#else
	p_node->mp_next = NULL;		
	if (!mp_concrete_metapiece_list)
	{
		mp_concrete_metapiece_list = p_node;
	}
	else
	{
		CMapListNode *p_add = mp_concrete_metapiece_list;
		while( p_add->mp_next)
		{
			p_add = p_add->mp_next;
		}
		p_add->mp_next = p_node;
	}
	#endif
	
	m_num_concrete_metapieces++;

	return p_meta;
}




void CParkManager::DestroyConcreteMeta(CConcreteMetaPiece *pMeta, EDestroyFlags flags)
{
	Dbg_Assert(m_state_on);
	if (pMeta->m_flags & CMetaPiece::mLOCK_AGAINST_DELETE) return;
	Dbg_MsgAssert(!(pMeta->m_flags & CMetaPiece::mLOCK_AGAINST_DELETE), ("Metapiece is locked, can't deleted"));

	if (CCursor::sInstance(false))
		CCursor::sInstance()->InformOfMetapieceDeletion(pMeta);
	
	// remove any gaps attached to this piece
	RemoveGap(pMeta);

	if (!pMeta->IsInnerShell() && pMeta->IsInPark())
	{
		decrement_meta_count(pMeta->GetNameChecksum());
		if (!pMeta->IsRiser())
		{
			m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~(mIN_SYNC_WITH_PARK | mIS_NEWER_THAN_PARK));
		}	
	}
	
	// destroy all metas that are ABOVE this one
	if (!pMeta->IsRiser() && (flags & mDESTROY_PIECES_ABOVE))
	{
		GridDims test_area = pMeta->GetArea();
		test_area[Y] = test_area.GetY() + test_area.GetH();
		test_area.MakeInfinitelyHigh();
		// XXX
		Ryan("die now: ");
		//test_area.PrintContents();
		DestroyMetasInArea(test_area, EDestroyFlags(mEXCLUDE_RISERS	| mDESTROY_PIECES_ABOVE));
	}
	
	apply_meta_contained_risers_to_floor(pMeta, false);
	
	debucketify_metapiece(pMeta);
	remove_metapiece_from_node_list(pMeta, &mp_concrete_metapiece_list);
	
	#if 0
	printf("concrete metapiece %s destroyed: cell dims (%d,%d,%d)-(%d,%d,%d), flags=0x%x\n",
		   Script::FindChecksumName(pMeta->GetNameChecksum()),
		   pMeta->m_cell_area.GetX(), pMeta->m_cell_area.GetY(), pMeta->m_cell_area.GetZ(), 
		   pMeta->m_cell_area.GetW(), pMeta->m_cell_area.GetH(), pMeta->m_cell_area.GetL(),
		   pMeta->m_flags);
	#endif

										 
	CParkGenerator::RestartType restart_type = IsRestartPiece(pMeta->GetNameChecksum());		
	if (restart_type != CParkGenerator::vEMPTY)
	{
		mp_generator->RemoveRestart(pMeta->GetArea(),restart_type);
	}
	
	// take out metapiece
	for (uint i = 0; i < pMeta->m_num_used_entries; i++)
	{
		CClonedPiece *p_piece = pMeta->get_desc_at_index(i).mpPiece->CastToCClonedPiece();
		Dbg_Assert(p_piece);
		mp_generator->DestroyClonedPiece(p_piece);
	}
	
	delete pMeta;
	m_num_concrete_metapieces--;
}


void CParkManager::HighlightMetasInArea(GridDims area, bool on)
{
	CMapListTemp list = GetAreaMetaPieces(area);
	CMapListNode *p_entry = list.GetList();

	while(p_entry)
	{
		CMetaPiece *p_meta=p_entry->GetMeta();
		if (p_meta)
		{
			if (p_meta->IsConcrete() && p_meta->IsCompletelyWithinArea(area))
			{
				p_entry->GetConcreteMeta()->Highlight(on);
			}	
		}	
		p_entry = p_entry->GetNext();
	}
}

/*
GridDims CParkManager::FindBoundingArea(GridDims area)
{
	int min_x=1000000;	
	int min_z=1000000;	
	int max_x=-1000000;
	int max_z=-1000000;
	
	CMapListTemp list = GetAreaMetaPieces(area);
	CMapListNode *p_entry = list.GetList();

	while(p_entry)
	{
		CMetaPiece *p_meta=p_entry->GetMeta();
		if (p_meta && p_meta->IsConcrete())
		{
			GridDims meta_area = p_entry->GetConcreteMeta()->GetArea();
			int x1=meta_area.GetX();
			int z1=meta_area.GetZ();
			int x2=x1+meta_area.GetW()-1;
			int z2=z1+meta_area.GetL()-1;
			
			if (x1 < min_x) min_x=x1;
			if (x2 > max_x) max_x=x2;
			if (z1 < min_z) min_z=z1;
			if (z2 > max_z) max_z=z2;
		}	
		p_entry = p_entry->GetNext();
	}
	
	GridDims bounding_area=area;
	if (min_x <= max_x && min_z <= max_z)
	{
		bounding_area.SetXYZ(min_x, 0, min_z);
		bounding_area.SetWHL(max_x-min_x+1, 1, max_z-min_z+1);
	}
	return bounding_area;
}
*/

// K: Does the same as CCursor::AttemptRemove() but for any area
bool CParkManager::DestroyMetasInArea(GridDims area, bool doNotDestroyRestartsOrFlags, bool onlyIfWithinArea)
{
	bool destroyed_something=false;
	CMapListTemp metas_at_pos = GetAreaMetaPieces(area);
	CMapListNode *p_entry = metas_at_pos.GetList();
	while (p_entry)
	{
		CConcreteMetaPiece *p_meta = p_entry->GetConcreteMeta();
		if (p_meta->IsRiser() || 
			(doNotDestroyRestartsOrFlags && p_meta->IsRestartOrFlag()) ||
			(onlyIfWithinArea && !p_meta->IsCompletelyWithinArea(area)))
		{
		}
		else
		{
			DestroyConcreteMeta(p_meta, CParkManager::mDESTROY_PIECES_ABOVE);
			destroyed_something=true;
		}	
		
		p_entry = p_entry->GetNext();
	}

	// Also destroy any rail points in the area.
	Mth::Vector corner_pos;
	Mth::Vector area_dims;
	corner_pos=Ed::CParkManager::Instance()->GridCoordinatesToWorld(area,&area_dims);
	Obj::GetRailEditor()->ClipRails(corner_pos[X], corner_pos[Z],
									corner_pos[X]+area_dims[X], corner_pos[Z]+area_dims[Z],
									Obj::CRailEditorComponent::DELETE_POINTS_INSIDE);
	
	if (destroyed_something)
	{
		RebuildFloor();
		return true;
	}
		
	return false;
}


/*
	Destroys concrete metapieces in specified area, according to flags.
	
	Flags:
	-mDESTROY_PIECES_ABOVE: if set, whenever a meta is destroyed, recursively
		destroy all metas above it
		
	Return value:
	upper 16 bits: number metas killed
	lower 16 bits: number pieces killed
*/
uint32 CParkManager::DestroyMetasInArea(GridDims area, EDestroyFlags flags)
{
	uint16 metas_killed = 0;
	uint16 pieces_killed = 0;
	CMapListNode *p_list = GetMetaPiecesAt(area, Mth::ROT_0, NULL, (flags & mEXCLUDE_RISERS));
	CMapListNode *p_node = p_list;
	while (p_node)
	{
		if (!(flags & mEXCLUDE_PIECES_MARKED_AS_SLID) || !(p_node->GetConcreteMeta()->m_flags & CMetaPiece::mMARK_AS_SLID))
		{
			metas_killed++;
			pieces_killed += p_node->GetConcreteMeta()->m_num_used_entries;
			DestroyConcreteMeta(p_node->GetConcreteMeta(), flags);
		}
		p_node = p_node->GetNext();
	}
	p_list->DestroyList();

	return ((metas_killed<<16) | pieces_killed);
}

int CParkManager::get_dma_usage(uint32 pieceChecksum)
{
	Script::CStruct *p_dma_usage=Script::GetStructure(CRCD(0x9547da8d,"DMA_Usage"),Script::ASSERT);
	int usage=1;
	p_dma_usage->GetInteger(pieceChecksum, &usage);
	return usage;
}	

void CParkManager::increment_meta_count(uint32 pieceChecksum)
{
	++m_num_general_concrete_metapieces;
	m_dma_piece_count += get_dma_usage(pieceChecksum);
	
	//printf("IncrementMetaCount '%s', %d   DMA=%d\n",Script::FindChecksumName(pieceChecksum),m_num_general_concrete_metapieces,m_dma_piece_count);
}

void CParkManager::decrement_meta_count(uint32 pieceChecksum)
{
	--m_num_general_concrete_metapieces;
	m_dma_piece_count -= get_dma_usage(pieceChecksum);
	
	//printf("DecrementMetaCount '%s' %d   DMA=%d\n",Script::FindChecksumName(pieceChecksum),m_num_general_concrete_metapieces,m_dma_piece_count);
	Dbg_MsgAssert(m_dma_piece_count >= 0,("Negative m_dma_piece_count of %d\n",m_dma_piece_count));
}

/*
	Position is in cell coordinates, relative to northwest corner of 256X256 grid.
	Y coordinate is relative to "ground level", can be + or -.

	Rotation is 0-3.
*/
void CParkManager::AddMetaPieceToPark(GridDims pos, CConcreteMetaPiece *pMetaPiece)
{
	Dbg_Assert(m_state_on);
	
	Dbg_MsgAssert(!(pMetaPiece->m_flags & CMetaPiece::mIN_PARK), ("metapiece already in park"));
	pMetaPiece->set_flag(CMetaPiece::mIN_PARK);

	pMetaPiece->m_cell_area.SetXYZ(pos.GetX(), pos.GetY(), pos.GetZ());

	// center_pos in world coordinates
	Mth::Vector center_pos;
	center_pos[X] = ((float) pos.GetX() + (float) pMetaPiece->m_cell_area.GetW() / 2.0f) * CParkGenerator::CELL_WIDTH;
	center_pos[X] -= (CParkGenerator::CELL_WIDTH * (float) CParkManager::FLOOR_MAX_WIDTH) / 2.0f;
	center_pos[Z] = ((float) pos.GetZ() + (float) pMetaPiece->m_cell_area.GetL() / 2.0f) * CParkGenerator::CELL_LENGTH;
	center_pos[Z] -= (CParkGenerator::CELL_LENGTH * (float) CParkManager::FLOOR_MAX_LENGTH) / 2.0f;
	center_pos[Y] = pos.GetY() * CParkGenerator::CELL_HEIGHT;

	// XXX
	#if 0
	Dbg_Printf("adding piece to park: (%d,%d,%d), (%.2f,%.2f,%.2f)\n", 
		 pos.GetX(), pos.GetY(), pos.GetZ(), 
		 center_pos[X], center_pos[Y], center_pos[Z]);
	#endif
	
	for (uint i = 0; i < pMetaPiece->m_num_used_entries; i++)
	{
		Mth::Vector rel_pos = pMetaPiece->GetRelativePosOfContainedPiece(i);
		CClonedPiece *p_piece = pMetaPiece->GetContainedPiece(i)->CastToCClonedPiece();
		
		// reposition piece from metapiece-relative coordinates to world-relative
		Mth::Vector abs_pos;
		abs_pos[X] = center_pos[X] + rel_pos[X]; 
		abs_pos[Z] = center_pos[Z] + rel_pos[Z]; 
		abs_pos[Y] = center_pos[Y] + rel_pos[Y];
		
		p_piece->SetDesiredPos(abs_pos, CClonedPiece::CHANGE_SECTOR);
		p_piece->SetDesiredRot(Mth::ERot90((p_piece->GetRot() + pMetaPiece->m_rot) & 3), CClonedPiece::CHANGE_SECTOR);
		mp_generator->AddClonedPieceToWorld(p_piece);
		mp_generator->CalculateLighting(p_piece);
	}

	apply_meta_contained_risers_to_floor(pMetaPiece, true);		
	bucketify_metapiece(pMetaPiece);

	// If it's a restart point, then add that to the list of restart points
	CParkGenerator::RestartType restart_type = IsRestartPiece(pMetaPiece->GetNameChecksum());		
	if (restart_type != CParkGenerator::vEMPTY)
	{
		mp_generator->AddRestart(center_pos,pMetaPiece->m_rot,pos,restart_type,false);
	}

	if (!pMetaPiece->IsInnerShell())
	{
		if (!pMetaPiece->IsRiser())
		{
			m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~(mIN_SYNC_WITH_PARK | mIS_NEWER_THAN_PARK));
		}
		increment_meta_count(pMetaPiece->GetNameChecksum());
	}
	
}




/*
	"Moves" a metapiece from one position to another. Really, the old metapiece is destroyed, and a new copy created,
	so be careful to replace any pointers to old one.
*/
CConcreteMetaPiece *CParkManager::RelocateMetaPiece(CConcreteMetaPiece *pMeta, int changeX, int changeY, int changeZ)
{
	GridDims area = pMeta->GetArea();
	uint32 name = pMeta->GetNameChecksum();
	Mth::ERot90 rot = pMeta->GetRot();
	CMetaPiece::EFlags old_flags = pMeta->m_flags;

	// get gap associated with metapiece
	int gap_half;
	bool gap_exists = false;
	CGapManager::GapDescriptor *p_gap_desc = GetGapDescriptor(area, &gap_half);
	CGapManager::GapDescriptor new_gap_desc;
	if (p_gap_desc)
	{
		gap_exists = true;
		new_gap_desc = *p_gap_desc;
		
		// remove it
		RemoveGap(*p_gap_desc);

		// shift gap descriptor to new position
		new_gap_desc.loc[gap_half][X] = new_gap_desc.loc[gap_half].GetX() + changeX;
		new_gap_desc.loc[gap_half][Y] = new_gap_desc.loc[gap_half].GetY() + changeY;
		new_gap_desc.loc[gap_half][Z] = new_gap_desc.loc[gap_half].GetZ() + changeZ;
	}
	
	DestroyConcreteMeta(pMeta, mDONT_DESTROY_PIECES_ABOVE);
	
	area[X] += changeX;
	area[Y] = area.GetY() + changeY;
	area[Z] += changeZ;

	CConcreteMetaPiece *p_new_meta = CreateConcreteMeta(GetAbstractMeta(name), false);
	p_new_meta->SetRot(rot);
	p_new_meta->m_flags = old_flags;
	p_new_meta->clear_flag(CMetaPiece::mIN_PARK);
	AddMetaPieceToPark(area, p_new_meta);

	if (gap_exists)
	{
		AddGap(new_gap_desc);
	}

	return p_new_meta;
}




/*
	Returns list of concrete metapieces in specified area.
	
	Modes:
	pPiece == NULL, grab all pieces in area specified by 'dims', excluding risers and riser-containing metas if desired.
	
	pPiece != NULL, does pretty much the same thing right now.
*/

CMapListNode *CParkManager::GetMetaPiecesAt(GridDims dims, Mth::ERot90 rot, CAbstractMetaPiece *pPiece, bool excludeRisers)
{
	Dbg_Assert(m_state_on);
	
	if (!pPiece)
	{
		CMapListNode *p_out_list = NULL;
		
		// need to be scanning an area bigger than 0
		//if (dims.mW == 0) dims.mW++;
		//if (dims.mL == 0) dims.mL++;
		
		// no piece supplied, just scan through and grab all metapieces
		int end_x = dims.GetX() + dims.GetW();
		for (int x = dims.GetX(); x < end_x; x++)
		{
			int end_z = dims.GetZ() + dims.GetL();
			for (int z = dims.GetZ(); z < end_z; z++)
			{
				CMapListNode *p_node = mp_bucket_list[x][z];
				
				while(p_node)
				{
					//Ryan("boogah %d\n", x);
					if ((!excludeRisers || !(p_node->GetMeta()->IsRiser())) &&
						p_node->GetConcreteMeta()->test_for_conflict(dims, excludeRisers))
					{
						add_metapiece_to_node_list(p_node->GetMeta(), &p_out_list);
					}
					
					p_node = p_node->GetNext();
				}
			}
		}
		
		p_out_list->VerifyIntegrity();
		return p_out_list;
	}
	else
	{
		Dbg_MsgAssert(pPiece->GetRot() == Mth::ROT_0, ("abstract metapiece needs 0 rotation"));
		
		// build a descriptor table rotated and shifted to correct world coordinates
		
		// rotate the abstract meta piece to the desired rotation
		pPiece->SetRot(rot);
		dims[W] = pPiece->m_cell_area.GetW();
		dims[H] = pPiece->m_cell_area.GetH();
		dims[L] = pPiece->m_cell_area.GetL();

		CMapListNode *p_out_list = NULL;
		
		// grab list of all concrete metas
		CMapListTemp metas_in_park = GetMetaPiecesAt(dims);

		// for each contained piece
		for (uint i = 0; i < pPiece->m_num_used_entries; i++)
		{
			SMetaDescriptor &desc = pPiece->get_desc_at_index(i);
			GridDims test_area(desc.mX, desc.mY, desc.mZ);
			test_area[X] += dims.GetX();
			test_area[Y] += dims.GetY();
			test_area[Z] += dims.GetZ();

			CMapListNode *p_in_park_entry = metas_in_park.GetList();
			while (p_in_park_entry)
			{
				if (p_in_park_entry->GetConcreteMeta()->test_for_conflict(test_area, false))
					add_metapiece_to_node_list(p_in_park_entry->GetMeta(), &p_out_list);

				p_in_park_entry = p_in_park_entry->GetNext();
			}
		}
		
		// return abstract meta to a zero rotation
		pPiece->SetRot(Mth::ROT_0);
		p_out_list->VerifyIntegrity();
		return p_out_list;
	}
	
	return NULL;
}

// This is just a slightly faster version of GetMetaPiecesAt, which gets all
CMapListNode *CParkManager::GetAreaMetaPieces(GridDims dims)
{
	Dbg_Assert(m_state_on);

	CMapListNode *p_out_list = NULL;
	
	int end_x = dims.GetX() + dims.GetW();
	for (int x = dims.GetX(); x < end_x; x++)
	{
		int end_z = dims.GetZ() + dims.GetL();
		for (int z = dims.GetZ(); z < end_z; z++)
		{
			CMapListNode *p_node = mp_bucket_list[x][z];
			while(p_node)
			{
				add_metapiece_to_node_list(p_node->GetMeta(), &p_out_list);
				p_node = p_node->GetNext();
			}
		}
	}
		
	return p_out_list;
}

// K: These two functions used by CClipboard::CopySelectionToClipboard
CMapListNode *CParkManager::GetMapListNode(int x, int z)
{
	return mp_bucket_list[x][z];
}

int CParkManager::GetFloorHeight(int x, int z)
{
	return m_floor_height_map[x][z].mHeight;
}
	
/*
	Used to determine if resizing park will destroy existing metas.
*/
bool CParkManager::AreMetasOutsideBounds(GridDims new_dims)
{
	//Ryan("New Park Bounds are: (%d,%d,%d)-(%d,%d,%d)\n", 
	//	 new_dims.GetX(), new_dims.GetY(), new_dims.GetZ(),
	//	 new_dims.GetW(), new_dims.GetH(), new_dims.GetL());
	CMapListNode *p_node = mp_concrete_metapiece_list;
	while(p_node)
	{
		CConcreteMetaPiece *p_meta = p_node->GetConcreteMeta();
		GridDims meta_area = p_meta->GetArea();
		//Ryan("Checking if meta in bounds: is_riser=%d, inner_shell=%d (%d,%d,%d)-(%d,%d,%d)\n", 
		//	 p_meta->IsRiser(), p_meta->IsInnerShell(), 
		//	 meta_area.GetX(), meta_area.GetY(), meta_area.GetZ(),
		//	 meta_area.GetW(), meta_area.GetH(), meta_area.GetL());
		if ((meta_area.GetX() < new_dims.GetX() ||
			meta_area.GetZ() < new_dims.GetZ() ||
			meta_area.GetX() + meta_area.GetW() > new_dims.GetX() + new_dims.GetW() ||
			meta_area.GetZ() + meta_area.GetL() > new_dims.GetZ() + new_dims.GetL()) &&
			!p_meta->IsRiser() &&
			!p_meta->IsInnerShell() &&
			(p_meta->m_flags & CMetaPiece::mIN_PARK))
			return true;
		p_node = p_node->GetNext();
	}

	return false;
}




/*
	Used to determine if resizing park will destroy existing metas.
*/
bool CParkManager::EnoughMemoryToResize(GridDims new_dims)
{
	int current_num_tiles = m_park_near_bounds.GetW() * m_park_near_bounds.GetL();
	int new_num_tiles = new_dims.GetW() * new_dims.GetL();

	if (new_num_tiles - current_num_tiles > GetGenerator()->GetResourceSize("max_dma_pieces") - GetDMAPieceCount())
	{
		return false;
	}
		
	int park_add_amount = (new_num_tiles - current_num_tiles) * mp_generator->GetResourceSize("floor_piece_size_park");
	int main_add_amount = (new_num_tiles - current_num_tiles) * mp_generator->GetResourceSize("floor_piece_size_main");

	CParkGenerator::MemUsageInfo usage_info = mp_generator->GetResourceUsageInfo();
	if (usage_info.mParkHeapFree < park_add_amount) 
	{
		Ryan("not enough memory for resize on park heap, need %d, have %d\n", park_add_amount, usage_info.mParkHeapFree);
		return false;
	}
	if (usage_info.mMainHeapFree < main_add_amount) 
	{
		Ryan("not enough memory for resize on main heap, need %d, have %d\n", main_add_amount, usage_info.mMainHeapFree);
		return false;
	}
	
	#ifdef __PLAT_NGC__
	// NGC has problems resizing if the top down heap is low due to UpdateSuperSectors requiring some temporary space.
	int required_top_down_heap=TOP_DOWN_REQUIRED_MARGIN;
	
	// If increasing the size of the park, then in the worse case the TD heap may reduce by main_add_amount,
	// if none of the fragments got used.
	// We want there to be more than TOP_DOWN_REQUIRED_MARGIN after increasing the size, so that the user
	// will be able to reduce the size afterwards, so add in main_add_amount to the requirement.
	if (main_add_amount > 0)
	{
		required_top_down_heap += main_add_amount;
	}	
	
	Mem::Heap *p_top_down_heap = Mem::Manager::sHandle().TopDownHeap();
	// Check if there is enough TD heap at the moment
	if (p_top_down_heap->mp_region->MemAvailable() < required_top_down_heap)
	{
		return false;
	}
	#endif
	
	return true;
}




/*
	Returns max floor height within specified
*/
int	CParkManager::GetFloorHeight(GridDims dims, bool *pRetUniformHeight)
{
	Dbg_Assert(m_state_on);
	Dbg_MsgAssert(m_floor_height_map, ("bad bad monkey!!!"));
	
	int max_height = -10000;
	
	if (pRetUniformHeight)
		*pRetUniformHeight = true;
	int corner_height = m_floor_height_map[dims.GetX()][dims.GetZ()].mHeight;
	
	for (int x = dims.GetX(); x < dims.GetX() + dims.GetW(); x++)
	{
		for (int z = dims.GetZ(); z < dims.GetZ() + dims.GetL(); z++)
		{
			Dbg_MsgAssert(x >= 0 && z >= 0 && x < FLOOR_MAX_WIDTH && z < FLOOR_MAX_LENGTH,
						  ("out of bounds (%d,%d)", x, z));
			if (pRetUniformHeight && m_floor_height_map[x][z].mHeight != corner_height)
				*pRetUniformHeight = false;
			if (m_floor_height_map[x][z].mHeight > max_height)
				max_height = m_floor_height_map[x][z].mHeight;
		}
	}
	
	return max_height;
}




#if 0
/*
	Changes the floor height stored in the height map, A subsequent call to
	generate_floor_pieces_from_height_map() will make the geometry.
*/
void CParkManager::SetFloorHeight(GridDims area, int newHeight, bool justDropHighest)
{
	for (int x = area.GetX(); x < area.GetX() + area.GetW(); x++)
	{
		for (int z = area.GetZ(); z < area.GetZ() + area.GetL(); z++)
		{
			Dbg_MsgAssert(x >= 0 && z >= 0 && x < FLOOR_MAX_WIDTH && z < FLOOR_MAX_LENGTH,
						  ("out of bounds (%d,%d)", x, z));
			
			GridDims test_area(x, 0, z, 1, 1, 1);
			if (IsOccupiedByRoad(test_area))
			{
				m_floor_height_map[x][z].mHeight = 0;
			}
			if (justDropHighest)
			{
				if (m_floor_height_map[x][z].mHeight > newHeight)
					m_floor_height_map[x][z].mHeight = newHeight;
			}
			else
				m_floor_height_map[x][z].mHeight = newHeight;
		}
	}
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~(mIN_SYNC_WITH_PARK | mIS_NEWER_THAN_PARK));
}
#endif


void CParkManager::ResetFloorHeights(GridDims area)
{
	// First move all the pieces in the area such that their y's are zero.
	CMapListTemp metas_at_pos = GetAreaMetaPieces(area);
	CMapListNode *p_entry = metas_at_pos.GetList();
	while (p_entry)
	{
		CConcreteMetaPiece *p_meta = p_entry->GetConcreteMeta();
		if (!p_meta->IsRiser())
		{
			GridDims meta_area=p_meta->GetArea();
			RelocateMetaPiece(p_meta,0,-meta_area.GetY(),0);
			
			// Set the floor heights under the piece to zero. Need to do this as well
			// as zeroing the heights for the whole area, in case the piece sticks out
			// of the area.
			for (int x = meta_area.GetX(); x < meta_area.GetX() + meta_area.GetW(); x++)
			{
				for (int z = meta_area.GetZ(); z < meta_area.GetZ() + meta_area.GetL(); z++)
				{
					Dbg_MsgAssert(x >= 0 && z >= 0 && x < FLOOR_MAX_WIDTH && z < FLOOR_MAX_LENGTH,
								  ("out of bounds (%d,%d)", x, z));
					m_floor_height_map[x][z].mHeight=0;
				}
			}		
		}
		
		p_entry = p_entry->GetNext();
	}

	// Then set all the floor heights to zero
	for (int x = area.GetX(); x < area.GetX() + area.GetW(); x++)
	{
		for (int z = area.GetZ(); z < area.GetZ() + area.GetL(); z++)
		{
			Dbg_MsgAssert(x >= 0 && z >= 0 && x < FLOOR_MAX_WIDTH && z < FLOOR_MAX_LENGTH,
						  ("out of bounds (%d,%d)", x, z));
			m_floor_height_map[x][z].mHeight=0;
		}
	}		

	// K: Do I need this?
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~(mIN_SYNC_WITH_PARK | mIS_NEWER_THAN_PARK));
	
	// Then rebuild the floor so that the above changes take effect.
	RebuildFloor();
}	


/*
	A "column" is a 3D region defined by the XZ rectangle of 'area'. Its bottom is at height 'y'. All the 
	metapieces within the column will change y position by the amount specified. Floor heights within the
	column will change as well.
	
	Returns true if success.
*/
bool CParkManager::SlideColumn(GridDims area, int bottom, int top, bool up, bool uneven)
{
	ESlideColumnFlags flags = ESlideColumnFlags(mFIRST_RECURSE);
	if (up) flags = ESlideColumnFlags(flags | mUP);
	if (uneven) flags = ESlideColumnFlags(flags | mUNEVEN);
	
	/* Remove bookkeeping left from last SlideColumn */
	finish_slide_column();
	if (mp_column_slide_list)
		mp_column_slide_list->DestroyList();
	mp_column_slide_list = NULL;
	m_num_metas_killed_in_slide = 0;
	m_num_pieces_killed_in_slide = 0;
	
	#if 1
	if (uneven && intersection_with_riser_containing_metas(area))
		return false;
	#endif
	
	if (!slide_column_part_one(area, bottom, top, flags, &mp_column_slide_list))
	{
		fix_slide_column_part_one_failure();
		return false;
	}
	slide_column_part_two(mp_column_slide_list);
	
	if (!up)
		kill_pieces_in_layer_under_area(area);
	
	return true;
}





/*
	Undoes the results of a successful SlideColumn()
*/
void CParkManager::UndoSlideColumn()
{
	fix_slide_column_part_one_failure();
	
	CMapListNode *p_node = mp_column_slide_list;
	while(p_node)
	{
		p_node->mp_meta = RelocateMetaPiece(p_node->GetConcreteMeta(), 0, -p_node->mSlideAmount, 0);
		p_node = p_node->GetNext();
	}
}

// K: This is copied from CCursor::ChangeFloorHeight
// Modified to take a GridDims parameter so that it can be used by the clipboard pasting code.
bool CParkManager::ChangeFloorHeight(GridDims dims, int dir)
{
	/*
		Uneven floor:
			If going up, raise to height of topmost
			If going down, only lower ones on same level
	*/

	bool uniform_height;
	int height = GetFloorHeight(dims, &uniform_height);

	/*
		Now, 
	*/

	bool success = true;
	if (SlideColumn(dims, dims.GetY(), height, (dir > 0), !uniform_height))
	{
		//Ryan("Slide column, dir = %d, dims=", dir);
		//dims.PrintContents();

		// K: This was required in the CCursor version of ChangeFloorHeight, can't
		// call it here though cos it is a member function of CCursor ...
		//change_cell_pos(0, 0);
		
		Spt::SingletonPtr<CParkEditor> p_editor;
		if (RebuildFloor(p_editor->IsParkFull()))
		{
			HighlightMetasWithGaps(false, NULL);
		}
		else
		{
			// out of memory, so reverse slide
			UndoSlideColumn();
			RebuildFloor();
			success = false;
		}
	}
	else
		success = false;

	return success;
}



/*
	Converts a cell based XYZ coordinate to its world coordinate equivalent.
*/
Mth::Vector	CParkManager::GridCoordinatesToWorld(const GridDims &cellDims, Mth::Vector *pRetDims)
{
	Mth::Vector world_pos(((int) cellDims.GetX() - (int) CParkManager::FLOOR_MAX_WIDTH / 2) * CParkGenerator::CELL_WIDTH,
		(int) cellDims.GetY() * CParkGenerator::CELL_HEIGHT,
		((int) cellDims.GetZ() - (int) CParkManager::FLOOR_MAX_LENGTH / 2) * CParkGenerator::CELL_LENGTH);

	if (pRetDims)
	{
		(*pRetDims)[X] = cellDims.GetW() * CParkGenerator::CELL_WIDTH;
		(*pRetDims)[Y] = cellDims.GetH() * CParkGenerator::CELL_HEIGHT;
		(*pRetDims)[Z] = cellDims.GetL() * CParkGenerator::CELL_LENGTH;
		#if 0
		printf("dims: (%d,%d,%d)-(%d,%d,%d)\n", 
			   cellDims.GetW(), cellDims.GetH(), cellDims.GetL(),
			   (int) cellDims.GetX() - (int) CParkManager::FLOOR_MAX_WIDTH / 2,
			   (int) cellDims.GetY(),
			   (int) cellDims.GetZ() - (int) CParkManager::FLOOR_MAX_LENGTH / 2);
		#endif
	}
	
	return world_pos;
}




CParkManager *CParkManager::sInstance()
{
	Dbg_Assert(sp_instance);
	return sp_instance;
}




/*
	The manager keeps track of a piece database that matches the selection set the user sees at the top of the screen.
*/
CParkManager::CPieceSet &CParkManager::GetPieceSet(int setNumber, int *pMenuSetNumber)
{
	Dbg_Assert(setNumber >= 0 && setNumber < MAX_SETS);
	
	int menu_set_number = 0;
	for (int s = 0; s < setNumber; s++)
	{
		if (m_palette_set[s].mVisible)
			menu_set_number++;
	}
	
	if (pMenuSetNumber)
		*pMenuSetNumber = menu_set_number;

	return m_palette_set[setNumber];
}




/*
	y coordinates, dimensions of 'area' not used.
*/
bool CParkManager::IsOccupiedByRoad(GridDims area)
{
	for (uint i = 0; i < m_road_mask_tab_size; i++)
	{
		bool overlap = true;
		if (area.GetX() + area.GetW() <= m_road_mask_tab[i].GetX()) overlap = false;
		else if (area.GetX() >= m_road_mask_tab[i].GetX() + m_road_mask_tab[i].GetW()) overlap = false;
		else if (area.GetZ() + area.GetL() <= m_road_mask_tab[i].GetZ()) overlap = false;
		else if (area.GetZ() >= m_road_mask_tab[i].GetZ() + m_road_mask_tab[i].GetL()) overlap = false;

		if (overlap) return true;
	}
	return false;
}

bool CParkManager::CanPlacePiecesIn(GridDims cellDims, bool ignoreFloorHeight)
{
	Spt::SingletonPtr<CParkEditor> p_editor;
	if (p_editor->IsParkFull())
	{
		ParkEd("p_editor->IsParkFull()");
		return false;
	}	

	// Check if there are any pieces in the area already.
	// Not using GetMetaPiecesAt because it does not detect pieces in lowered cells for some reason.
	CMapListTemp metas_at_pos = GetAreaMetaPieces(cellDims);
	CMapListNode *p_entry = metas_at_pos.GetList();
	while (p_entry)
	{
		CConcreteMetaPiece *p_meta=p_entry->GetConcreteMeta();
		if (!p_meta->IsRiser())
		{
			return false;
		}
			
		p_entry = p_entry->GetNext();
	}
	
	if (!ignoreFloorHeight)
	{
		bool uniform_floor;
		int floor_height = GetFloorHeight(cellDims, &uniform_floor);
		if (floor_height != cellDims.GetY() || !uniform_floor)
		{
			ParkEd("floor_height != cellDims.GetY() || !uniform_floor");
			return false;
		}	
	}
		
	if (IsOccupiedByRoad(cellDims))
	{
		ParkEd("IsOccupiedByRoad(cellDims)");
		return false;
	}	
	
	return true;
}


// pHalf returns the half number associated with the piece, if any gap found
CGapManager::GapDescriptor *CParkManager::GetGapDescriptor(GridDims &area, int *pHalf)
{
	area.SetWHL(1,1,1);
	CMapListTemp meta_list = GetMetaPiecesAt(area);
	if (meta_list.IsEmpty())
		return NULL;
	CConcreteMetaPiece *p_meta = meta_list.GetList()->GetConcreteMeta();

	int tab_index;
	Dbg_Assert(mp_gap_manager);
	if (!mp_gap_manager->IsGapAttached(p_meta, &tab_index))
		return NULL;
	
	CGapManager::GapDescriptor *pDesc = mp_gap_manager->GetGapDescriptor(tab_index, pHalf);
	Dbg_MsgAssert(pDesc, ("couldn't find gap descriptor"));
	return pDesc;
}




/*
	Call from:
	
	read_from_compressed_map_buffer(), CParkEditor, gap adjustment code, gap name changer
	
	-see if meta at cursor loc
	-if user working on gap already, fail
	-if no prexisiting, remove gap-in-progress
	-build descriptor
	-add gap
*/
bool CParkManager::AddGap(CGapManager::GapDescriptor &descriptor)
{
	Dbg_Assert(mp_gap_manager);
	int gap_tab_index = mp_gap_manager->GetFreeGapIndex();
	if (gap_tab_index == -1) return false;

	for (int i = 0; i < descriptor.numCompleteHalves; i++)
	{
		// fetch piece associated with this gap half
		descriptor.loc[i].SetWHL(1,1,1);
		CMapListTemp meta_list = GetMetaPiecesAt(descriptor.loc[i]);
		if (meta_list.IsEmpty())
			return false;
		Dbg_MsgAssert(!meta_list.IsEmpty(), ("am expecting a metapiece here"));
		CConcreteMetaPiece *p_meta = meta_list.GetList()->GetConcreteMeta();
		Dbg_MsgAssert(p_meta, ("no pieces found at (%d,%d,%d)", descriptor.loc[i].GetX(), descriptor.loc[i].GetY(), descriptor.loc[i].GetZ()));
		Dbg_MsgAssert(!mp_gap_manager->IsGapAttached(p_meta, NULL), ("gap is already attached"));
		p_meta->set_flag(CMetaPiece::mGAP_ATTACHED);
		
		descriptor.loc[i] = p_meta->GetArea();
		
		#if 0
		Ryan("  Placing gap half %d, loc=(%d,%d,%d-%d,%d,%d), rot=%d, left=%d, right=%d\n", i,
			 descriptor.loc[i].GetX(), descriptor.loc[i].GetY(), descriptor.loc[i].GetZ(), 
			 descriptor.loc[i].GetW(), descriptor.loc[i].GetH(), descriptor.loc[i].GetL(),
			 descriptor.rot[i], descriptor.leftExtent[i], descriptor.rightExtent[i]);
		#endif
		
		GridDims area = p_meta->GetArea();
		
		#if 1
		// work out gap positioning
		Mth::Vector gap_pos;
		float gap_length = 0.0f;
		int gap_rot = 0;
		switch(descriptor.rot[i])
		{
			case 0:	// east
				gap_length = (area.GetL() + descriptor.leftExtent[i] + descriptor.rightExtent[i]) * CParkGenerator::CELL_LENGTH;
				gap_pos[X] = (area.GetX() + area.GetW()) * CParkGenerator::CELL_WIDTH - CParkGenerator::CELL_WIDTH / 2.0f - 1.0f;
				gap_pos[Z] = (area.GetZ() - descriptor.rightExtent[i]) * CParkGenerator::CELL_LENGTH + gap_length / 2.0f - CParkGenerator::CELL_LENGTH;
				gap_pos[Y] = area.GetY() * CParkGenerator::CELL_HEIGHT;
				gap_rot = 1;
				break;
			case 1:	// north
				gap_length = (area.GetW() + descriptor.leftExtent[i] + descriptor.rightExtent[i]) * CParkGenerator::CELL_WIDTH;
				gap_pos[X] = (area.GetX() - descriptor.rightExtent[i]) * CParkGenerator::CELL_WIDTH + gap_length / 2.0f - CParkGenerator::CELL_WIDTH;
				gap_pos[Z] = area.GetZ() * CParkGenerator::CELL_LENGTH - CParkGenerator::CELL_LENGTH / 2.0f + 1.0f;
				gap_pos[Y] = area.GetY() * CParkGenerator::CELL_HEIGHT;
				gap_rot = 0;
				break;
			case 2:	// west
				gap_length = (area.GetL() + descriptor.leftExtent[i] + descriptor.rightExtent[i]) * CParkGenerator::CELL_LENGTH;
				gap_pos[X] = area.GetX() * CParkGenerator::CELL_WIDTH - CParkGenerator::CELL_WIDTH / 2.0f + 1.0f;
				gap_pos[Z] = (area.GetZ() - descriptor.leftExtent[i]) * CParkGenerator::CELL_LENGTH + gap_length / 2.0f - CParkGenerator::CELL_LENGTH;
				gap_pos[Y] = area.GetY() * CParkGenerator::CELL_HEIGHT;
				gap_rot = 1;
				break;
			case 3:	// south
				gap_length = (area.GetW() + descriptor.leftExtent[i] + descriptor.rightExtent[i]) * CParkGenerator::CELL_WIDTH;
				gap_pos[X] = (area.GetX() - descriptor.leftExtent[i]) * CParkGenerator::CELL_WIDTH + gap_length / 2.0f - CParkGenerator::CELL_WIDTH;
				gap_pos[Z] = (area.GetZ() + area.GetL()) * CParkGenerator::CELL_LENGTH - CParkGenerator::CELL_LENGTH / 2.0f - 1.0f;
				gap_pos[Y] = area.GetY() * CParkGenerator::CELL_HEIGHT;
				gap_rot = 0;
				break;
			default:
				Dbg_MsgAssert(0, ("not a valid gap rotation"));
				break;
		}
		#endif
		
		gap_pos[X] -= MAX_WIDTH * CParkGenerator::CELL_WIDTH / 2.0f;
		gap_pos[Z] -= MAX_LENGTH * CParkGenerator::CELL_LENGTH / 2.0f;

		CClonedPiece *p_gap_piece = mp_generator->CreateGapPiece(gap_pos, gap_length, gap_rot);
		p_gap_piece->SetScaleX(gap_length / CParkGenerator::CELL_WIDTH);
		
		// create gap
		if (i == 0)
			mp_gap_manager->StartGap(p_gap_piece, p_meta, gap_tab_index);
		else	
			mp_gap_manager->EndGap(p_gap_piece, p_meta, gap_tab_index);
	}	

	descriptor.tabIndex = gap_tab_index;
	mp_gap_manager->SetGapInfo(gap_tab_index, descriptor);

	return true;
}



/*
	Called from: change cursor, cleanup, gap placement, gap adjustment, name change
*/
void CParkManager::RemoveGap(CGapManager::GapDescriptor &descriptor)
{
	#if 1
	// XXX
	for (int half = 0; half < 2; half++)
		Ryan("   removing gap half %d (%d,%d,%d),(%d,%d,%d) left=%d right=%d, index=%d\n", half,
			 descriptor.loc[half].GetX(), descriptor.loc[half].GetY(), descriptor.loc[half].GetZ(),
			 descriptor.loc[half].GetW(), descriptor.loc[half].GetH(), descriptor.loc[half].GetL(),
			 descriptor.leftExtent[half], descriptor.rightExtent[half],
				descriptor.tabIndex);
	#endif
	
	CConcreteMetaPiece *p_meta[2];
	for (int half = 0; half < descriptor.numCompleteHalves; half++)
	{
		CMapListTemp meta_list = GetMetaPiecesAt(descriptor.loc[half]);
		Dbg_MsgAssert(!meta_list.IsEmpty(), ("no pieces found at (%d,%d,%d)", descriptor.loc[half].GetX(), descriptor.loc[half].GetY(), descriptor.loc[half].GetZ()));
		
		#if 1
		if (!meta_list.IsSingular())
		{
//			printf("failed singularity, half %d\n", half);
			meta_list.PrintContents();
		}
		#endif
		
		// go through list, find metapiece matching location above
		// there is only SUPPOSED to be one metapiece in list, but sometimes there are more
		// and I have no idea why, so fuck it
		CMapListNode *p_node = meta_list.GetList();
		while(p_node)
		{
			p_meta[half] = p_node->GetConcreteMeta();
			GridDims area = p_meta[half]->GetArea();

			if (area.GetX() == descriptor.loc[half].GetX() &&
				area.GetY() == descriptor.loc[half].GetY() &&
				area.GetZ() == descriptor.loc[half].GetZ())
				break;

			p_node = p_node->GetNext();
		}

		//p_meta[half] = meta_list.GetList()->GetConcreteMeta();
		p_meta[half]->Highlight(false);
		p_meta[half]->clear_flag(CMetaPiece::mGAP_ATTACHED);
	}

	int tab_index;
	Dbg_Assert(mp_gap_manager);
	if (mp_gap_manager->IsGapAttached(p_meta[0], &tab_index))
	{
		mp_gap_manager->RemoveGap(tab_index);
	}

	descriptor.tabIndex = -1;
	// Reset the cancel flags to the default, to fix TT9493
	descriptor.mCancelFlags=Script::GetInteger(CRCD(0x9a27e74d,"Cancel_Ground"));
}




void CParkManager::RemoveGap(CConcreteMetaPiece *pMeta)
{
	int tab_index;
	Dbg_Assert(mp_gap_manager);
	if (!mp_gap_manager->IsGapAttached(pMeta, &tab_index))
		return;
	
	int half;
	CGapManager::GapDescriptor *pDesc = mp_gap_manager->GetGapDescriptor(tab_index, &half);
	Dbg_MsgAssert(pDesc, ("couldn't find gap descriptor"));
	RemoveGap(*pDesc);
}




void CParkManager::HighlightMetasWithGaps(bool highlightOn, CGapManager::GapDescriptor *pDesc)
{
	CMapListNode *p_node = mp_concrete_metapiece_list;
	while (p_node)
	{
		if (p_node->GetConcreteMeta()->m_flags & CMetaPiece::mGAP_ATTACHED)
			p_node->GetConcreteMeta()->Highlight(highlightOn, true);
		p_node = p_node->GetNext();
	}

	mp_generator->SetGapPiecesVisible(false);
	
	if (highlightOn && pDesc && pDesc->tabIndex >= 0)
	{
		mp_gap_manager->MakeGapWireframe(pDesc->tabIndex);
	}
	else if (!highlightOn)
	{
		mp_generator->SetGapPiecesVisible(false);
	}
}

// Added for use by s_generate_summary_info in mcfunc.cpp
void CParkManager::GetSummaryInfoFromBuffer(uint8 *p_buffer, int *p_numGaps, int *p_numMetas, uint16 *p_theme, uint32 *p_todScript, int *p_width, int *p_length)
{
	CompressedMapHeader *p_header=(CompressedMapHeader*)p_buffer;
	*p_width=p_header->mW;
	*p_length=p_header->mL;
	*p_numGaps=p_header->mNumGaps;
	*p_numMetas=p_header->mNumMetas;
	*p_theme=p_header->mTheme;
	if (p_header->mTODScript)
	{
		*p_todScript=p_header->mTODScript;
	}
	else
	{
		*p_todScript=CRCD(0x1ca1ff20,"default");
	}	
}


/*
	Creates a "stack" of risers within the specified vertical column. If !simpleBuild, will keep exisiting
	riser metapieces if they do the job.
	
	'top' refers to cell position of top of topmost riser
*/
void CParkManager::output_riser_stack(BuildFloorParams &params)
{
	Dbg_MsgAssert(m_floor_height_map, ("bad bad monkey!!!"));
	
	Dbg_MsgAssert(params.top >= -16 && params.top <= 15, ("faulty floor height %d, at (%d,%d)", params.top, params.x, params.z));
	
	int lowest_neighbor = 10000;
	
	// work out lowest neighbor
	if (params.x > m_park_far_bounds.GetX())
	{
		if (m_floor_height_map[params.x-1][params.z].mHeight < lowest_neighbor)
			lowest_neighbor = m_floor_height_map[params.x-1][params.z].mHeight;
	}
	else
		lowest_neighbor = (lowest_neighbor > 0) ? 0 : lowest_neighbor;

	if (params.x < m_park_far_bounds.GetX() + m_park_far_bounds.GetW() - 1)
	{
		if (m_floor_height_map[params.x+1][params.z].mHeight < lowest_neighbor)
			lowest_neighbor = m_floor_height_map[params.x+1][params.z].mHeight;
	}
	else
		lowest_neighbor = (lowest_neighbor > 0) ? 0 : lowest_neighbor;

	if (params.z > m_park_far_bounds.GetZ())
	{
		if (m_floor_height_map[params.x][params.z-1].mHeight < lowest_neighbor)
			lowest_neighbor = m_floor_height_map[params.x][params.z-1].mHeight;
	}
	else
		lowest_neighbor = (lowest_neighbor > 0) ? 0 : lowest_neighbor;

	if (params.z < m_park_far_bounds.GetZ() + m_park_far_bounds.GetL() - 1)
	{
		if (m_floor_height_map[params.x][params.z+1].mHeight < lowest_neighbor)
			lowest_neighbor = m_floor_height_map[params.x][params.z+1].mHeight;
	}
	else
		lowest_neighbor = (lowest_neighbor > 0) ? 0 : lowest_neighbor;

	EFloorFlags flags = mHAS_BOTTOM;
	GridDims road_test_area(params.x, 0, params.z, 1, 1, 1);
	if (IsOccupiedByRoad(road_test_area))
	{
		flags = EFloorFlags(flags | mNO_COVER);
		params.top = 0;
	}
	
	if (lowest_neighbor < params.top)
	{
		params.bottom = lowest_neighbor;
	}
	else
	{
		params.bottom = params.top-1;
		flags = EFloorFlags(flags & ~mHAS_BOTTOM);
	}

	params.flags = flags;
	for (int y = params.bottom; y < params.top; y++)
	{
		if (y == params.top-1 && !(flags & mNO_COVER))
			params.flags = EFloorFlags(params.flags | mHAS_TOP);
		
		if (y == -1)
		{
			if (y <= lowest_neighbor)
				params.heightSlotOffset = 3; // group 4 -- bottom level, just below surface
			else
				params.heightSlotOffset = 2; // group 3 -- not bottom level, just below surface
		}
		else if (y < -1)
		{
			if (y <= lowest_neighbor)
				params.heightSlotOffset = 0; // group 1 -- bottom level, not touching surface
			else
				params.heightSlotOffset = 1; // group 2 -- not bottom level, not touching surface
		}
		else
		{
			if (y == params.top-1)
			{
				if (y == 0)
					params.heightSlotOffset = 4; // group 5 -- top level risers sitting on surface
				else
					params.heightSlotOffset = 7; // group 8 -- risers sitting above surface, top level
			}
			else if (y == 0)
				params.heightSlotOffset = 5; // group 6 -- risers sitting on surface, but not top level
			else
				params.heightSlotOffset = 6; // group 7 -- risers sitting above surface, not top level
		}		
		
		params.y = y;
		if (params.simpleBuild)
			build_add_floor_piece_simple(params); 
		else
			build_add_floor_piece(params); 
	}
}




/*
	Creates a riser metapiece, if one is needed.
*/
void CParkManager::build_add_floor_piece(BuildFloorParams &params)
{
	uint32 type = TYPE_EMPTY_BLOCK;
	if (params.flags & mHAS_BOTTOM)
	{
		if (params.flags & mHAS_TOP)
			type = m_riser_piece_checksum[params.heightSlotOffset][0];
		else
			type = m_riser_piece_checksum[params.heightSlotOffset][1];
	}
	else if (params.flags & mHAS_TOP)
	{
		type = m_riser_piece_checksum[params.heightSlotOffset][2];
	}
	else
		return;
	
	// see if floor already included in metapiece at this position
	int floor_height = params.y + 1;
	Dbg_MsgAssert(floor_height >= -16 && floor_height <= 15, ("faulty floor height %d, at (%d,%d,%d)", 
															  floor_height, params.x, params.y, params.z));
	uint32 floor_bit = (1<<(floor_height+16));
	if (m_floor_height_map[params.x][params.z].mEnclosedFloor & floor_bit)
	{
		#if 0
		printf("preexisting riser at %d (%d,%d)!\n", floor_height, params.x, params.z);
		#endif
		return;
	}
	
	// see if floor piece is already there and if it has desired type
	GridDims test_dims(params.x, params.y, params.z, 1, 1, 1);
	CMapListNode *p_node = GetMetaPiecesAt(test_dims);
	if (p_node)
	{
		if (p_node->GetMeta()->m_name_checksum == type)
		{
			/*
			if (type == TYPE_FLOOR_BLOCK && 0)
				printf("preserving floor block at %d,%d,%d\n", 
					   p_node->GetMeta()->GetArea().GetX(),
					   p_node->GetMeta()->GetArea().GetY(),
					   p_node->GetMeta()->GetArea().GetZ());
			*/
			
			// undo mark for kill
			p_node->GetMeta()->clear_flag(CMetaPiece::mMARK_FOR_KILL);
			p_node->DestroyList();
			return;
		}
		else
			p_node->DestroyList();
	}
	
	if (!params.dryRun)
	{
		#ifdef USE_BUILD_LIST
		enum
		{
			MAX_BUILD_LIST_SIZE=10000,
		};
		Dbg_MsgAssert(m_build_list_size<MAX_BUILD_LIST_SIZE-1,("Build list overflow"));

		if (!mp_build_list_entry)
		{
			Dbg_MsgAssert(m_build_list_size==0,("Expected m_build_list_size to be 0"));
			#ifndef	__USE_EXTERNAL_BUFFER__			
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().FrontEndHeap());
			mp_build_list_entry=(SBuildListEntry*)Mem::Malloc(MAX_BUILD_LIST_SIZE * sizeof(SBuildListEntry));
			Mem::Manager::sHandle().PopContext();
			#else
			ACQUIRE_BUFFER
			mp_build_list_entry = (SBuildListEntry*)__EXTERNAL_BUFFER__;
			#endif
		}		
		mp_build_list_entry[m_build_list_size].mType=type;
		GridDims pos(params.x, params.y, params.z);
		mp_build_list_entry[m_build_list_size].mPos=pos;
		++m_build_list_size;
		#else
		CConcreteMetaPiece *p_out = CreateConcreteMeta(GetAbstractMeta(type));
		GridDims pos(params.x, params.y, params.z);
		AddMetaPieceToPark(pos, p_out);
		#endif

		
		#if 0
		if (type == TYPE_FLOOR_BLOCK)
			printf("adding floor block at %d,%d,%d\n", 
				   params.x,
				   params.y,
				   params.z);
	
		if (type == TYPE_FLOOR_WALL_BLOCK)
			printf("adding floor wall block at %d,%d,%d\n", 
				   params.x,
				   params.y,
				   params.z);
		#endif
	}
	else
	{
		params.addCount2 += GetAbstractMeta(type)->m_num_used_entries;
	}
	params.addCount++;
}




/*
	Creates a riser metapiece, period.
*/
void CParkManager::build_add_floor_piece_simple(BuildFloorParams &params)
{
	uint32 type = TYPE_EMPTY_BLOCK;
	if (params.flags & mHAS_BOTTOM)
	{
		if (params.flags & mHAS_TOP)
			type = m_riser_piece_checksum[params.heightSlotOffset][0];
		else
			type = m_riser_piece_checksum[params.heightSlotOffset][1];
	}
	else if (params.flags & mHAS_TOP)
	{
		type = m_riser_piece_checksum[params.heightSlotOffset][2];
	}
	else
		return;
	
	
	CConcreteMetaPiece *p_out = CreateConcreteMeta(GetAbstractMeta(type));
	GridDims pos(params.x, params.y, params.z);
	AddMetaPieceToPark(pos, p_out);
	params.addCount++;
}




/*
	Some metapieces contain risers.
*/
void CParkManager::apply_meta_contained_risers_to_floor(CConcreteMetaPiece *pMetaPiece, bool add)
{
	for (uint i = 0; i < pMetaPiece->m_num_used_entries; i++)
	{
		CClonedPiece *p_piece = pMetaPiece->GetContainedPiece(i)->CastToCClonedPiece();
		
		// see if this piece affects floor height
		SMetaDescriptor &desc = pMetaPiece->get_desc_at_index(i);
		if (desc.mRiser)
		{
			GridDims riser_area;
			p_piece->GetCellDims(&riser_area);
			riser_area[X] = pMetaPiece->m_cell_area.GetX() + desc.mX;
			riser_area[Y] = pMetaPiece->m_cell_area.GetY() + desc.mY;
			riser_area[Z] = pMetaPiece->m_cell_area.GetZ() + desc.mZ;
	
			int start_x = riser_area.GetX();
			int end_x = start_x + riser_area.GetW();
			int start_z = riser_area.GetZ();
			int end_z = start_z + riser_area.GetL();
			#if 0
			printf("raising floor at: (%d,%d,%d)+(%d,%d,%d), desc.mRiser is %d at 0x%x, index %d\n",
				   pMetaPiece->m_cell_area.GetX(), pMetaPiece->m_cell_area.GetY(), pMetaPiece->m_cell_area.GetZ(),
				   desc.mX, desc.mY, desc.mZ, desc.mRiser, &desc.mRiser, i);
			#endif
			for (int x = start_x; x < end_x; x++)
			{
				for (int z = start_z; z < end_z; z++)
				{
					Dbg_MsgAssert(x >= 0 && z >= 0 && x < FLOOR_MAX_WIDTH && z < FLOOR_MAX_LENGTH,
								  ("out of bounds (%d,%d)", x, z));
					if (add)
					{
						if (m_floor_height_map[x][z].mHeight < riser_area.GetY() + riser_area.GetH())
							m_floor_height_map[x][z].mHeight = riser_area.GetY() + riser_area.GetH();
						if (m_floor_height_map[x][z].mHeight > 15)
							m_floor_height_map[x][z].mHeight = 15;
						if (m_floor_height_map[x][z].mHeight < -16)
							m_floor_height_map[x][z].mHeight = -16;
					}
					else
					{
						// Are there non-riser metapieces above this one?
						GridDims area_above = riser_area;
						area_above[Y] = riser_area.GetY() + riser_area.GetH();
						area_above.MakeInfinitelyHigh();
						CMapListTemp meta_list = GetMetaPiecesAt(area_above, Mth::ROT_0, NULL, true);
						if (meta_list.IsEmpty())
						{
							if (m_floor_height_map[x][z].mHeight > riser_area.GetY())
								m_floor_height_map[x][z].mHeight = riser_area.GetY();
						}
					}

					for (int y = riser_area.GetY(); y < riser_area.GetY() + riser_area.GetH(); y++)
					{
						if (add)
							m_floor_height_map[x][z].mEnclosedFloor |= (1<<(y+16+1));
						else
							m_floor_height_map[x][z].mEnclosedFloor &= ~(1<<(y+16+1));
					}
					
					#if 0
					printf("  floor change: (%d,%d) to %d, flags 0x%x, bottom %d, top %d\n", 
						   x, z, m_floor_height_map[x][z].mHeight, m_floor_height_map[x][z].mEnclosedFloor,
						   riser_area.GetY(), riser_area.GetY() + riser_area.GetH());
					#endif
				}
			}
		}
	}
}




bool CParkManager::intersection_with_riser_containing_metas(GridDims area)
{
	area.MakeInfiniteOnY();
	
	CMapListTemp list = GetMetaPiecesAt(area, Mth::ROT_0, NULL, true);
	CMapListNode *p_node = list.GetList();
	while (p_node)
	{
		if (p_node->GetConcreteMeta()->m_flags & CMetaPiece::mCONTAINS_RISERS)
			return true;
		p_node = p_node->GetNext();
	}

	return false;
}




/*
	Change map height. Add metapieces in column to move list. Repeat this for every metapiece in move list. No
	metapiece will end up in move list twice, though.
	
	Good LUCK figuring this out. :)
*/
bool CParkManager::slide_column_part_one(GridDims area, int bottom, int top, ESlideColumnFlags flags, CMapListNode **ppMoveList)
{
	GridDims test_area = area;
	test_area[Y] = (uint8) bottom;
	if (flags & mFIRST_RECURSE)
		test_area.MakeInfiniteOnY();
	else
		test_area.MakeInfinitelyHigh();

	for (int x = test_area.GetX(); x < test_area.GetX() + test_area.GetW(); x++)
	{
		for (int z = test_area.GetZ(); z < test_area.GetZ() + test_area.GetL(); z++)
		{
			Dbg_MsgAssert(x >= 0 && z >= 0 && x < FLOOR_MAX_WIDTH && z < FLOOR_MAX_LENGTH,
						  ("out of bounds (%d,%d)", x, z));
			if (!m_floor_height_map[x][z].mMarkAsSlid)
			{
				int change_amount = 0;
				if (flags & mUNEVEN)
				{
					if (flags & mUP)
						change_amount = top - m_floor_height_map[x][z].mHeight;
					else
						change_amount = (m_floor_height_map[x][z].mHeight == top) ? -1 : 0;
				}
				else
				{
					if (flags & mUP)
						change_amount = 1;
					else
						change_amount = -1;
				}
				
				m_floor_height_map[x][z].mHeight += change_amount;
				m_floor_height_map[x][z].mSlideAmount = change_amount;

				if (m_floor_height_map[x][z].mHeight < -16 || m_floor_height_map[x][z].mHeight > 15)
				{
					return false;
				}
			}
			m_floor_height_map[x][z].mMarkAsSlid = true;
		}
	}

	CMapListTemp list = GetMetaPiecesAt(test_area, Mth::ROT_0, NULL, true);
	CMapListNode *p_node = list.GetList();
	while (p_node)
	{
		CConcreteMetaPiece *p_meta = p_node->GetConcreteMeta();
		GridDims meta_area = p_meta->GetArea();
		
		if (!p_meta->IsRiser() && !(p_meta->m_flags & CMetaPiece::mMARK_AS_SLID))
		{
			/*
				Are we actually going to move this meta?
				
				Even surface:
				-yes
				
				Uneven
				-up: only if lower than top
				-down
			*/
			
			p_meta->set_flag(CMetaPiece::mMARK_AS_SLID);
			Mem::Manager::sHandle().PushContext(mp_generator->GetParkEditorHeap());
			CMapListNode *p_new_node = new CMapListNode();
			Mem::Manager::sHandle().PopContext();
			p_new_node->mp_next = *ppMoveList;
			p_new_node->mp_meta = p_meta;			
			*ppMoveList = p_new_node;

			#if 0
			Ryan("rising meta at: ");
			p_meta->GetArea().PrintContents();
			#endif
			
			p_new_node->mSlideAmount = 0;
			if (flags & mUNEVEN)
			{
				if (flags & mUP)
					p_new_node->mSlideAmount = top - meta_area.GetY();
				else
					p_new_node->mSlideAmount = (meta_area.GetY() == top) ? -1 : 0;
			}
			else
			{
				if (flags & mUP)
					p_new_node->mSlideAmount = 1;
				else
					p_new_node->mSlideAmount = -1;
			}
			
			ESlideColumnFlags recurse_flags = ESlideColumnFlags(flags & ~mFIRST_RECURSE);
			if (!slide_column_part_one(meta_area, meta_area.GetY(), top, recurse_flags, ppMoveList))
				return false;
		}
		p_node = p_node->GetNext();
	}

	return true;
}




void CParkManager::fix_slide_column_part_one_failure()
{
	for (int x = 0; x < FLOOR_MAX_WIDTH; x++)
	{
		for (int z = 0; z < FLOOR_MAX_LENGTH; z++)
		{
			if (m_floor_height_map[x][z].mMarkAsSlid)
			{
				m_floor_height_map[x][z].mMarkAsSlid = false;
				m_floor_height_map[x][z].mHeight -= m_floor_height_map[x][z].mSlideAmount;
			}
			if (m_floor_height_map[x][z].mHeight > 15)
				m_floor_height_map[x][z].mHeight = 15;
			if (m_floor_height_map[x][z].mHeight < -16)
				m_floor_height_map[x][z].mHeight = -16;
		}
	}
}




/*
	Go through move list, relocate each metapiece
*/
void CParkManager::slide_column_part_two(CMapListNode *pMoveList)
{
	//Mem::Heap *p_top_down_heap = Mem::Manager::sHandle().TopDownHeap();
	
	CMapListNode *p_node = pMoveList;
	while (p_node)
	{
		CConcreteMetaPiece *p_meta = p_node->GetConcreteMeta();
		
		if (p_node->mSlideAmount < 0)
		{
			kill_pieces_in_layer_under_area(p_meta->GetArea());
		}

		p_node->mp_meta = RelocateMetaPiece(p_meta, 0, p_node->mSlideAmount, 0);

		//if (p_top_down_heap->mp_region->MemAvailable() < 200000)
		//{
		//	GetGenerator()->GetClonedScene()->UpdateSuperSectors();
		//}	
		
		p_node = p_node->GetNext();
	}	
}




void CParkManager::kill_pieces_in_layer_under_area(GridDims area)
{
	GridDims kill_area = area;
	kill_area[Y] = area.GetY() - 1;
	kill_area[H] = 1;

	#if 0
	printf("killing metas in area: (%d,%d,%d)-(%d,%d,%d)\n",
		   kill_area.GetX(), kill_area.GetY(), kill_area.GetZ(),
		   kill_area.GetW(), kill_area.GetH(), kill_area.GetL());
	#endif

	uint32 killed_result = DestroyMetasInArea(kill_area, mEXCLUDE_PIECES_MARKED_AS_SLID);
	m_num_metas_killed_in_slide += killed_result>>16;
	m_num_pieces_killed_in_slide += killed_result & 0xFFFF;
}




/*
	Undo 'slid' markers in floor map array. Turn off MARK_AS_SLID flags on metapieces.
*/
void CParkManager::finish_slide_column()
{
	for (int x = m_park_far_bounds.GetX(); x < m_park_far_bounds.GetX() + m_park_far_bounds.GetW(); x++)
	{
		for (int z = m_park_far_bounds.GetZ(); z < m_park_far_bounds.GetZ() + m_park_far_bounds.GetL(); z++)
		{
			Dbg_MsgAssert(x >= 0 && z >= 0 && x < FLOOR_MAX_WIDTH && z < FLOOR_MAX_LENGTH,
						  ("out of bounds (%d,%d)", x, z));
			m_floor_height_map[x][z].mMarkAsSlid = false;
			m_floor_height_map[x][z].mSlideAmount = 0;
		}
	}

	CMapListNode *p_node = mp_concrete_metapiece_list;
	while(p_node)
	{
		p_node->GetConcreteMeta()->clear_flag(CMetaPiece::mMARK_AS_SLID);
		p_node = p_node->GetNext();
	}
}

#ifdef USE_BUILD_LIST
void CParkManager::create_metas_in_build_list()
{
	for (int i=0; i<m_build_list_size; ++i)
	{
		Dbg_MsgAssert(mp_build_list_entry,("NULL mp_build_list_entry"));
		CConcreteMetaPiece *p_out = CreateConcreteMeta(GetAbstractMeta(mp_build_list_entry[i].mType));
		AddMetaPieceToPark(mp_build_list_entry[i].mPos, p_out);
	}	
	m_build_list_size=0;
	if (mp_build_list_entry)
	{
		#ifndef	__USE_EXTERNAL_BUFFER__
		Mem::Free(mp_build_list_entry);
		#endif
		mp_build_list_entry=NULL;
	}	
}
#endif

/*
	Turns the height map into actual riser geometry. Creates all the CConcreteMetaPieces that are needed, destroys 
	the ones that aren't. If an existing CConcreteMetaPiece matches the height map, it is preserved.
	
	The simpleBuild parameter causes all riser metapieces to be destroyed and rebuilt. Use this parameter
	when you know you'll be doing this anyway.
	
	dryRun means don't actually create or destroy pieces, just make a count
*/
bool CParkManager::generate_floor_pieces_from_height_map(bool simpleBuild, bool dryRun)
{
	ParkEd("CParkManager::generate_floor_pieces_from_height_map(simpleBuild = %d)", simpleBuild);
	
	Dbg_Assert(m_state_on);
	
	//printf("=============== generate_floor_pieces_from_height_map() ========================\n");

	// mark all existing floor pieces for destruction (some will be unmarked later)
	int mark_count = 0;	// counts all risers existing upon entering this function
	CMapListNode *p_node = mp_concrete_metapiece_list;
	while(p_node)
	{
		CMetaPiece *p_meta = p_node->GetMeta();
		if (p_meta->IsRiser())
		{
			p_meta->set_flag(CMetaPiece::mMARK_FOR_KILL);
			mark_count++;
		}
		p_node = p_node->GetNext();
	}
	mark_count += m_num_metas_killed_in_slide;

	#ifdef USE_BUILD_LIST
	m_build_list_size=0;
	Dbg_MsgAssert(mp_build_list_entry==NULL,("Non-NULL mp_build_list_entry"));
	#endif

	
	Dbg_MsgAssert(m_floor_height_map, ("bad bad monkey!!!"));
	BuildFloorParams riser_build_params;
	riser_build_params.simpleBuild = simpleBuild;
	riser_build_params.dryRun = dryRun;
	riser_build_params.addCount = 0; // counts all risers added by the output_riser_stack() calls
	riser_build_params.addCount2 = 0;
	for (int x = m_park_near_bounds.GetX() - 1; x < m_park_near_bounds.GetX() + m_park_near_bounds.GetW() + 1; x++)
	{
		for (int z = m_park_near_bounds.GetZ() - 1; z < m_park_near_bounds.GetZ() + m_park_near_bounds.GetL() + 1; z++)
		{
			Dbg_MsgAssert(x >= 0 && z >= 0 && x < FLOOR_MAX_WIDTH && z < FLOOR_MAX_LENGTH,
						  ("out of bounds (%d,%d)", x, z));
			riser_build_params.x = x;
			riser_build_params.z = z;
			riser_build_params.top = m_floor_height_map[x][z].mHeight;
			riser_build_params.bottom = m_park_far_bounds.GetY();
			output_riser_stack(riser_build_params);
		}

		#ifdef __PLAT_XBOX__
		// On the XBox the large amount of CPU time taken up by rebuilding the park causes a glitch when
		// wma music is playing. So make sure Pcm::Update gets called often whilst in this inner loop.
		DirectSoundDoWork();
		Pcm::Update();
		#endif // __PLAT_XBOX__
		#ifdef __PLAT_NGC__
		// Same for the NGC ...
		Pcm::Update();
		#endif // __PLAT_NGC__
	}

	// remove all marked floor piece
	int kill_count = 0; // counts risers killed
	int kill_count2 = 0; // counts contained pieces
	int left_count = 0;	// counts risers not killed plus those added
	p_node = mp_concrete_metapiece_list;
	while(p_node)
	{
		CMapListNode *p_nextNode = p_node->GetNext();		// get next node now, as DestroyConcretMeta, below, can delete current node
		CMetaPiece *p_meta = p_node->GetMeta();
		if (p_meta->m_flags & CMetaPiece::mMARK_FOR_KILL)
		{
			kill_count++;
			kill_count2 += p_meta->m_num_used_entries;
			if (!dryRun)
			{
				//p_node->GetConcreteMeta()->GetArea().PrintContents();
				DestroyConcreteMeta(p_node->GetConcreteMeta(), mDONT_DESTROY_PIECES_ABOVE);
			}
		}
		else if (p_meta->IsRiser())
			left_count++;
		p_node = p_nextNode;
	}
	kill_count += m_num_metas_killed_in_slide;
	kill_count2 += m_num_pieces_killed_in_slide;
	

	#ifdef USE_BUILD_LIST
	mp_generator->GetClonedScene()->UpdateSuperSectors();	
	if (!dryRun)
	{
		left_count+=m_build_list_size;
		create_metas_in_build_list();
	}
	Dbg_MsgAssert(mp_build_list_entry==NULL,("Non-NULL mp_build_list_entry"));
	#endif
		
	// risers preexisting + risers added - risers killed == risers left
	Dbg_Assert(dryRun || riser_build_params.addCount == left_count + kill_count - mark_count);
	
	//printf("riser_build_params.addCount = %d kill_count = %d, kill_count2 = %d\n",riser_build_params.addCount,kill_count,kill_count2);
	
	//if (dryRun)
	//	printf("DRYRUN: ");
	//printf("marked %d pieces, killed %d, not killed %d, left %d, added %d\n", 
	//	   mark_count, kill_count, mark_count - kill_count, left_count, riser_build_params.addCount);
	//printf("addCount2 = %d, killCount2 = %d\n", riser_build_params.addCount2, kill_count2);

	if (dryRun && (riser_build_params.addCount > kill_count || riser_build_params.addCount2 > kill_count2))
	{
		return false;
	}
	
	//if (dryRun && (riser_build_params.addCount - kill_count > 200))
	//{
	//	// Don't allow height changes that will result in too many pieces being created, to prevent
	//	// running out of memory.
	//	return false;
	//}

	return true;
}




/*
	Create new list if none
*/
void CParkManager::add_metapiece_to_node_list(CMetaPiece *pMetaToAdd, CMapListNode **ppList)
{
	CMapListNode *p_test_node = *ppList;
	while(p_test_node)
	{
		if (p_test_node->GetMeta() == pMetaToAdd) return; // it's in there
		p_test_node = p_test_node->GetNext();
	}

	// not in list already, so add it
	Mem::Manager::sHandle().PushContext(mp_generator->GetParkEditorHeap());
	CMapListNode *p_new_node = new CMapListNode();
	Mem::Manager::sHandle().PopContext();
	p_new_node->mp_meta = pMetaToAdd;
	p_new_node->mp_next = *ppList;
	*ppList = p_new_node;
}




/*
	Opposite of above
*/
void CParkManager::remove_metapiece_from_node_list(CMetaPiece *pMetaToRemove, CMapListNode **ppList)
{
	CMapListNode *p_node = *ppList;
	CMapListNode *p_prev = NULL;
	while(p_node)
	{
		if (p_node->GetMeta() == pMetaToRemove)
		{
			if (p_prev)
				p_prev->mp_next = p_node->GetNext();
			else
				*ppList = p_node->GetNext();

			delete p_node;
			return;
		}
		
		p_prev = p_node;
		p_node = p_node->GetNext();
	}
}




void CParkManager::bucketify_metapiece(CConcreteMetaPiece *pMeta)
{
	for (int x = 0; x < pMeta->m_cell_area.GetW(); x++)
	{
		for (int z = 0; z < pMeta->m_cell_area.GetL(); z++)
		{
			Mem::Manager::sHandle().PushContext(mp_generator->GetParkEditorHeap());
			CMapListNode *p_new_entry = new CMapListNode();
			Mem::Manager::sHandle().PopContext();
			p_new_entry->mp_meta = pMeta;
			
			p_new_entry->mp_next = mp_bucket_list[x + pMeta->m_cell_area.GetX()][z + pMeta->m_cell_area.GetZ()];
			mp_bucket_list[x + pMeta->m_cell_area.GetX()][z + pMeta->m_cell_area.GetZ()] = p_new_entry;
		}
	}
}




void CParkManager::debucketify_metapiece(CConcreteMetaPiece *pMeta)
{
	for (int x = 0; x < pMeta->m_cell_area.GetW(); x++)
	{
		for (int z = 0; z < pMeta->m_cell_area.GetL(); z++)
		{
			CMapListNode *p_prev = NULL;
			CMapListNode *p_entry = mp_bucket_list[x + pMeta->m_cell_area.GetX()][z + pMeta->m_cell_area.GetZ()];
			while(p_entry)
			{
				// piece should occur only once per list
				
				if (p_entry->mp_meta == pMeta)
				{
					if (p_prev)
						p_prev->mp_next = p_entry->mp_next;
					else
						mp_bucket_list[x + pMeta->m_cell_area.GetX()][z + pMeta->m_cell_area.GetZ()] = p_entry->mp_next;
					delete p_entry;
					break;
				}
	
				p_prev = p_entry;
				p_entry = p_entry->mp_next;
			}
		} // end for z
	} // end for x
}




void CParkManager::create_abstract_metapieces()
{
	ParkEd("CParkManager::create_abstract_metapieces()");
	
	/*
		This add_later stuff is so we can create singular metapieces that weren't defined in 
		Ed_standard_metapieces. Every CPiece that exists must have a corresponding singular
		metapiece, at least an abstract one.
	*/
	uint32 add_later_tab[1000];
	uint add_later_count = 0;
	
	Script::CArray *p_metapiece_array = Script::GetArray("Ed_standard_metapieces", Script::ASSERT);
	
	// this first loop makes sure that metapieces named in EdPieces2.q actually exist
	for (uint i = 0; i < p_metapiece_array->GetSize(); i++)
	{
		Script::CStruct *p_entry = p_metapiece_array->GetStructure(i);
		
		// see if singular or multiple metapiece
		uint32 single_crc = 0;
		if (p_entry->GetChecksum("single", &single_crc))
		{
			mp_generator->GetMasterPiece(single_crc, false);
		}		
	} // end for


	// now, build abstract metapieces from Ed_standard_metapieces array
	for (uint i = 0; i < p_metapiece_array->GetSize(); i++)
	{
		Script::CStruct *p_entry = p_metapiece_array->GetStructure(i);
		
		Mem::Manager::sHandle().PushContext(mp_generator->GetParkEditorHeap());
		CAbstractMetaPiece *p_meta = new CAbstractMetaPiece();
		Mem::Manager::sHandle().PopContext();
		
		// see if singular or multiple metapiece
		uint32 single_crc = 0;
		Script::CArray *p_multiple_array = NULL;
		if (p_entry->GetChecksum("single", &single_crc))
		{
			p_meta->initialize_desc_table(1);
			
			CSourcePiece *p_source_piece = mp_generator->GetMasterPiece(single_crc, true);
			
			GridDims area(0, 0, 0);
			p_source_piece->GetCellDims(&area);
			Script::CArray *p_pos_array = NULL;
			if (p_entry->GetArray("pos", &p_pos_array))
			{
				area.SetXYZ(p_pos_array->GetInteger(0), p_pos_array->GetInteger(1), p_pos_array->GetInteger(2));
			}

			p_meta->add_piece(p_source_piece, &area);

			// if just a singular meta, use the name checksum already associated with single piece
			p_meta->m_name_checksum = p_source_piece->GetType();
			Dbg_Assert(add_later_count < 1000);
			add_later_tab[add_later_count++] = p_meta->m_name_checksum;			
			p_meta->set_flag(CMetaPiece::mSINGULAR);
			
			// Finally, Check to see if it is a special type
			// and add it to the table of special types: 
			
			uint32 special_type;
			if (p_entry->GetChecksum(CRCD(0xf176a9fd,"special_type"), &special_type))
			{

				// gap piece detection here ----- need to reinstate this when we do gaps				
				if (special_type == CRCD(0x2813ab8a,"gap_placement"))
					p_meta->set_flag(CMetaPiece::mGAP_PLACEMENT);
				if (special_type == CRCD(0xfc26b6f4,"area_selection"))
					p_meta->set_flag(CMetaPiece::mAREA_SELECTION);
				if (special_type == CRCD(0xffd81c08,"rail_placement"))
					p_meta->set_flag(CMetaPiece::mRAIL_PLACEMENT);
				
				if (special_type == CRCD(0x48fa9c7b,"restart_1"))
					SetRestartTypeId(CParkGenerator::vONE_PLAYER, single_crc);
				if (special_type == CRCD(0x419183b2,"restart_multi"))
					SetRestartTypeId(CParkGenerator::vMULTIPLAYER, single_crc);
				if (special_type == CRCD(0xe69aefaf,"restart_horse"))
					SetRestartTypeId(CParkGenerator::vHORSE, single_crc);
				if (special_type == CRCD(0xa091ce25,"king_of_hill"))
					SetRestartTypeId(CParkGenerator::vKING_OF_HILL, single_crc);
				
				if (special_type == CRCD(0x74b0f4e9,"red_flag"))
				{
					SetRestartTypeId(CParkGenerator::vRED_FLAG, single_crc);
					p_meta->set_flag(CMetaPiece::mNOT_ON_GC);
				}
				if (special_type == CRCD(0xc893acf8,"green_flag"))
				{
					SetRestartTypeId(CParkGenerator::vGREEN_FLAG, single_crc);
					p_meta->set_flag(CMetaPiece::mNOT_ON_GC);
				}
				if (special_type == CRCD(0x40bdfa7e,"blue_flag"))
				{
					SetRestartTypeId(CParkGenerator::vBLUE_FLAG, single_crc);
					p_meta->set_flag(CMetaPiece::mNOT_ON_GC);
				}
				if (special_type == CRCD(0x3ae047fc,"yellow_flag"))
				{
					SetRestartTypeId(CParkGenerator::vYELLOW_FLAG, single_crc);
					p_meta->set_flag(CMetaPiece::mNOT_ON_GC);
				}
					
			}
		}
		else if (p_entry->GetArray("multiple", &p_multiple_array))
		{
			bool contains_risers = false;
			p_meta->initialize_desc_table(p_multiple_array->GetSize());
			
			for (uint j = 0; j < p_multiple_array->GetSize(); j++)
			{
				Script::CStruct *p_name_pos = p_multiple_array->GetStructure(j);
				
				Script::CComponent *p_name_crc_comp = p_name_pos->GetNextComponent(NULL);
				Dbg_Assert(p_name_crc_comp->mType == ESYMBOLTYPE_NAME);				
				uint32 name_crc = p_name_crc_comp->mChecksum;

				Script::CArray *p_pos_array = NULL;
				p_name_pos->GetArray(NONAME, &p_pos_array, Script::ASSERT);
				
				CSourcePiece *p_source_piece = mp_generator->GetMasterPiece(name_crc, true);
				Dbg_Assert(add_later_count < 1000);
				add_later_tab[add_later_count++] = name_crc;
				
				GridDims area;
				p_source_piece->GetCellDims(&area);
				area.SetXYZ(p_pos_array->GetInteger(0), p_pos_array->GetInteger(1), p_pos_array->GetInteger(2));
	
				Mth::ERot90 rot = Mth::ROT_0;
				if (p_pos_array->GetSize() > 3)
					rot = (Mth::ERot90) p_pos_array->GetInteger(3);
				
				if (p_name_pos->ContainsFlag(CRCD(0xcd75a3f,"riser")))
					contains_risers = true;
				p_meta->add_piece(p_source_piece, &area, rot, p_name_pos->ContainsFlag(CRCD(0xcd75a3f,"riser")));
			} // end for

			if (contains_risers)
				p_meta->set_flag(CMetaPiece::mCONTAINS_RISERS);
		}
		else if (p_entry->GetArray("ack", &p_multiple_array))
		{
			p_meta->initialize_desc_table(p_multiple_array->GetSize());
			
			for (uint j = 0; j < p_multiple_array->GetSize(); j++)
			{
				Script::CStruct *p_name_pos = p_multiple_array->GetStructure(j);
				
				Script::CComponent *p_name_crc_comp = p_name_pos->GetNextComponent(NULL);
				Dbg_Assert(p_name_crc_comp->mType == ESYMBOLTYPE_NAME);				
				uint32 name_crc = p_name_crc_comp->mChecksum;

				Script::CArray *p_pos_array = NULL;
				p_name_pos->GetArray(NONAME, &p_pos_array, Script::ASSERT);
				
				CAbstractMetaPiece *p_source_meta = GetAbstractMeta(name_crc);
				
				GridDims area = p_source_meta->GetArea();
				area.SetXYZ(p_pos_array->GetInteger(0), p_pos_array->GetInteger(1), p_pos_array->GetInteger(2));
	
				Mth::ERot90 rot = Mth::ROT_0;
				if (p_pos_array->GetSize() > 3)
					rot = (Mth::ERot90) p_pos_array->GetInteger(3);
				
				p_meta->add_piece_dumb(p_source_meta, &area, rot, p_name_pos->ContainsFlag(CRCD(0xcd75a3f,"riser")));
			} // end for
		}
		else
		{
			Dbg_MsgAssert(0, ("screwy entry %d in Ed_standard_metapieces", i));
		}
		
		#if 1
		#ifdef __NOPT_ASSERT__
		GridDims test_area = p_meta->GetArea();
		Dbg_MsgAssert(test_area.GetW() > 0 && test_area.GetH() > 0 && test_area.GetL() > 0, 
					  ("metapiece %s has invalid dimensions (%d,%d,%d)", Script::FindChecksumName(p_meta->GetNameChecksum()),
					   test_area.GetW(), test_area.GetH(), test_area.GetL()));
		#if 0
		if (p_meta->GetNameChecksum() == Script::GenerateCRC("Sk4Ed_Fence_10x10"))
			printf("OY! metapiece %s has dimensions (%d,%d,%d)\n", Script::FindChecksumName(p_meta->GetNameChecksum()),
					   test_area.GetW(), test_area.GetH(), test_area.GetL());
		#endif

		#endif		
		#endif
		
		// the name provided in EdPieces.q, if any, takes precedence
		if (p_entry->GetChecksum("name", &p_meta->m_name_checksum))
		{
			// this metapiece has been renamed, so we must treat it as a non-singular piece
			p_meta->clear_flag(CMetaPiece::mSINGULAR);
		}
		// make sure no piece with checksum currently exists
		Dbg_MsgAssert(!GetAbstractMeta(p_meta->m_name_checksum), ("name checksum already used for metapiece"));

		if (p_entry->ContainsFlag("is_riser"))
			p_meta->set_flag(CMetaPiece::mIS_RISER);
		if (p_entry->ContainsFlag("no_rails"))
		{
#			ifdef __NOPT_ASSERT__
			Ryan("No rails for %s\n", Script::FindChecksumName(p_meta->m_name_checksum));
#			endif
			p_meta->set_flag(CMetaPiece::mNO_RAILS);
		}
		
		Mem::Manager::sHandle().PushContext(mp_generator->GetParkEditorHeap());
		CMapListNode *p_node = new CMapListNode();
		Mem::Manager::sHandle().PopContext();
		p_node->mp_meta = p_meta;
		p_node->mp_next = mp_abstract_metapiece_list;
		mp_abstract_metapiece_list = p_node;
		m_num_abstract_metapieces++;
	} // end for

	
	for (uint i = 0; i < add_later_count; i++)
	{
		if (GetAbstractMeta(add_later_tab[i]))
			continue;
		
		Mem::Manager::sHandle().PushContext(mp_generator->GetParkEditorHeap());
		CAbstractMetaPiece *p_meta = new CAbstractMetaPiece();
		Mem::Manager::sHandle().PopContext();
		
		p_meta->initialize_desc_table(1);
		CSourcePiece *p_source_piece = mp_generator->GetMasterPiece(add_later_tab[i], true);

		GridDims area(0, 0, 0);
		p_source_piece->GetCellDims(&area);
		p_meta->add_piece(p_source_piece, &area);

		p_meta->m_name_checksum = add_later_tab[i];
		p_meta->set_flag(CMetaPiece::mSINGULAR);

		Mem::Manager::sHandle().PushContext(mp_generator->GetParkEditorHeap());
		CMapListNode *p_node = new CMapListNode();
		Mem::Manager::sHandle().PopContext();
		p_node->mp_meta = p_meta;
		p_node->mp_next = mp_abstract_metapiece_list;
		mp_abstract_metapiece_list = p_node;
		m_num_abstract_metapieces++;
	}
}




void CParkManager::SetRestartTypeId(CParkGenerator::RestartType type, uint32 id)
{

	// Mick	
//	printf ("SetRestartTypeId(%d,%x)\n",type,id);	
	if (type > CParkGenerator::vEMPTY && type < CParkGenerator::vNUM_RESTART_TYPES)
	{
		m_restartPieceIdTab[type] = id;		
	}

/*	
	switch(type)
	{
		case CParkGenerator::vONE_PLAYER:
//			printf ("Found vONE_PLAYER\n");
			m_restartPieceIdTab[0] = id;
			break;
		case CParkGenerator::vMULTIPLAYER:
//			printf ("Found vMULTIPLAYER\n");
			m_restartPieceIdTab[1] = id;
			break;
		case CParkGenerator::vHORSE:
//			printf ("Found vHORSE\n");
			m_restartPieceIdTab[2] = id;
			break;
		case CParkGenerator::vKING_OF_HILL:
//			printf ("Found vKING_OF_HILL\n");
			m_restartPieceIdTab[3] = id;
			break;
		default:
			break;
	
	}
*/
}




// returns the type of restart piece this is, or vEMPTY is it is not a restart piece
// (vEMPTY is 0, so the return value can be treated as true/false)
CParkGenerator::RestartType CParkManager::IsRestartPiece(uint32 id)
{
	for (int type = CParkGenerator::vONE_PLAYER; type < CParkGenerator::vNUM_RESTART_TYPES; type++)
	{
		if (id == m_restartPieceIdTab[type])
		{
			return (CParkGenerator::RestartType) type;
		}
	}
	return CParkGenerator::vEMPTY;

/*
	if (id == m_restartPieceIdTab[0])
		return CParkGenerator::vONE_PLAYER;
	if (id == m_restartPieceIdTab[1])
		return CParkGenerator::vMULTIPLAYER;
	if (id == m_restartPieceIdTab[2])
		return CParkGenerator::vHORSE;
	if (id == m_restartPieceIdTab[3])
		return CParkGenerator::vKING_OF_HILL;
	return CParkGenerator::vEMPTY;
*/
}

/*
Commented out cos I just discovered Destroy() does pretty much the same thing!

// K: This is for freeing up as much memory as possible when playing a park, so that
// the number of players can be increased. Each player requires 830K.
void CParkManager::FreeUpMemoryForPlayingPark()
{
	// Delete everything in the park editor heap, so that it can be removed, freeing
	// up 900K.
	
	// First delete the cursor.
	Ed::CParkEditor::Instance()->DeleteCursor();
	
	printf("After deleting cursor ...\n");	
	printf("Num CMapListNode's = %d\n",Ed::CMapListNode::SGetNumUsedItems());
	MemView_AnalyzeHeap(mp_generator->GetParkEditorHeap());

	mp_generator->DeleteSourceAndClonedPieces();
	
	// Delete the concrete metapieces
	CMapListNode *p_node = mp_concrete_metapiece_list;
	while(p_node)
	{
		CMapListNode *p_next_node = p_node->GetNext();
		delete p_node->GetConcreteMeta();
		delete p_node;
		p_node = p_next_node;
	}
	m_num_concrete_metapieces=0;
	mp_concrete_metapiece_list=NULL;

	printf("After deleting CParkManager::mp_concrete_metapiece_list ...\n");	
	printf("Num CMapListNode's = %d\n",Ed::CMapListNode::SGetNumUsedItems());
	MemView_AnalyzeHeap(Ed::CParkManager::sInstance()->GetGenerator()->GetParkEditorHeap());

	// Free up the abstract metapieces.	
	p_node = mp_abstract_metapiece_list;
	while(p_node)
	{
		CMapListNode *p_next_node = p_node->GetNext();
		delete p_node->GetAbstractMeta();
		delete p_node;
		p_node = p_next_node;
	}
	m_num_abstract_metapieces=0;
	mp_abstract_metapiece_list=NULL;


	printf("After deleting CParkManager::mp_abstract_metapiece_list ...\n");	
	printf("Num CMapListNode's = %d\n",Ed::CMapListNode::SGetNumUsedItems());
	MemView_AnalyzeHeap(Ed::CParkManager::sInstance()->GetGenerator()->GetParkEditorHeap());


	// The park editor heap is now totally empty!
	// So delete it.
	mp_generator->DeleteParkEditorHeap();
	
	
	
	printf("After deleting stuff ...\n");	
	printf("Num CMapListNode's = %d\n",Ed::CMapListNode::SGetNumUsedItems());
	MemView_AnalyzeHeap(Ed::CParkManager::sInstance()->GetGenerator()->GetParkEditorHeap());
}
*/

// K: Added type so that DESTROY_ONLY_PIECES can be passed when this function is used to clean
// up the park editor heap for deletion when playing a park.
void CParkManager::destroy_concrete_metapieces(CParkGenerator::EDestroyType type)
{
	ParkEd("CParkManager::destroy_concrete_metapieces()");

	Dbg_Assert(mp_gap_manager);
	mp_gap_manager->RemoveAllGaps();
	
	// Commented these out because destroy_concrete_metapieces gets called via 
	// ScriptFreeUpMemoryForPlayingPark, which must not delete the edited rails.
	// (Fix to TT2125)
	//Obj::GetRailEditor()->DestroyRailGeometry();
	//Obj::GetRailEditor()->DestroyPostGeometry();
	
	// remove all concrete metapieces, except locked ones
	CMapListNode *p_node = mp_concrete_metapiece_list;
	while(p_node)
	{
		CMapListNode *p_next_node = p_node->GetNext();
		DestroyConcreteMeta(p_node->GetConcreteMeta(), mDONT_DESTROY_PIECES_ABOVE);
		p_node = p_next_node;
	}
	
	mp_generator->DestroyAllClonedPieces(type); // if any are left
	
	mp_generator->KillRestarts();		// This should have been taken care of by destrying the actual concrete metas, but just to be safe....
}


#ifdef __PLAT_NGC__
void CParkManager::SwapMapBufferEndianness()
{
	CompressedMapHeader *p_header = (CompressedMapHeader *)mp_compressed_map_buffer;
	
	p_header->mChecksum=_32(p_header->mChecksum);
	p_header->mSizeInBytes=_16(p_header->mSizeInBytes);
	p_header->mVersion=_16(p_header->mVersion);
	p_header->mTheme=_16(p_header->mTheme);
	p_header->mParkSize=_16(p_header->mParkSize);
	p_header->mNumMetas=_16(p_header->mNumMetas);
	p_header->mNumGaps=_16(p_header->mNumGaps);
	p_header->mTODScript=_32(p_header->mTODScript);
	
	uint32 *p_metas=(uint32*)((uint8*)(p_header+1) + FLOOR_MAX_WIDTH * FLOOR_MAX_LENGTH);
	for (int i=0; i<p_header->mNumMetas; ++i)
	{
		p_metas[0]=_32(p_metas[0]);
		p_metas[1]=_32(p_metas[1]);
		p_metas+=2;
	}
	
	CompressedGap *p_gap=(CompressedGap*)p_metas;
	for (int i=0; i<p_header->mNumGaps; ++i)
	{
		p_gap->score=_16(p_gap->score);
		p_gap->mCancelFlags=_32(p_gap->mCancelFlags);
		++p_gap;
	}
}
#endif

/*
Park editor map file format:

Header
-size
-version
-theme
-X, Z, W, L
-number of metas

Floor height array, each entry:
-height
-texture
(included risers taken care of by placed metapieces)

Meta entries:
-10 bits: index
-8 bits: X
-8 bits: Z
-6 bits: padding
-2 bits: rot
-5 bits: Y
-1 bit: padding
(total: 40)

Gap info

User-defined stamps
-number contained
	-source meta
	-8 bits: X
	-8 bits: Z
	-6 bits: Y
	-2 bits: rot
*/

void CParkManager::write_compressed_map_buffer()
{
	Dbg_Assert(m_state_on);
	ParkEd("CParkManager::write_compressed_map_buffer()");	
	//DumpUnwindStack( 20, 0 );
	
	if (m_compressed_map_flags & mIS_NEWER_THAN_PARK)
		// we just loaded this park, so don't touch it
		return;
	
	for (int i=0; i<COMPRESSED_MAP_SIZE; ++i)
	{
		mp_compressed_map_buffer[i]=0;
	}
	
	Script::CArray *p_save_map_array = Script::GetArray("Ed_Save_Map", Script::ASSERT);
	uint save_map_array_size = p_save_map_array->GetSize();
	
	CompressedMapHeader *p_header = (CompressedMapHeader *) mp_compressed_map_buffer;
	p_header->mVersion = (VERSION);
	p_header->mTheme = (m_theme);
	p_header->mParkSize = (0);
	p_header->mX = m_park_near_bounds.GetX();
	p_header->mZ = m_park_near_bounds.GetZ();
	p_header->mW = m_park_near_bounds.GetW();
	p_header->mL = m_park_near_bounds.GetL();
	p_header->mTODScript = Ed::CParkEditor::Instance()->GetTimeOfDayScript();
	p_header->mMaxPlayers=GetGenerator()->GetMaxPlayers();
	Dbg_MsgAssert(strlen(mp_park_name) < 64,("Park name '%s' too long",mp_park_name));
	strcpy(p_header->mParkName,mp_park_name);

	CompressedFloorEntry *p_floor_entry = (CompressedFloorEntry *) (p_header + 1);		
	for (int i = 0; i < FLOOR_MAX_WIDTH; i++)
	{
		for (int j = 0; j < FLOOR_MAX_LENGTH; j++)
		{
			p_floor_entry->mHeight = (sint8) m_floor_height_map[i][j].mHeight;
			p_floor_entry++;
		}
	}

	uint8 *p_meta_entry = (uint8 *) p_floor_entry;

	int num_metas_outputted = 0;
	CMapListNode *p_meta_node = mp_concrete_metapiece_list;
	while(p_meta_node)
	{
		CConcreteMetaPiece *p_meta = p_meta_node->GetConcreteMeta();
		if (!p_meta->IsRiser() && !p_meta->IsInnerShell() && p_meta->IsInPark())
		{
			uint32 *p_meta_entry_first = (uint32 *) p_meta_entry;
			uint32 *p_meta_entry_second = (uint32 *) (p_meta_entry + 4);

			GridDims meta_area = p_meta->GetArea();
			
			uint32 metapiece_index = 0;
			uint32 metapiece_name_checksum = p_meta->GetNameChecksum();
			for (; metapiece_index < save_map_array_size; metapiece_index++)
			{	
				if (p_save_map_array->GetChecksum(metapiece_index) == metapiece_name_checksum)
					break;
			}
			
			if (metapiece_index < save_map_array_size)
			{			
				*p_meta_entry_first = metapiece_index & 1023;
				*p_meta_entry_first |= meta_area.GetX()	<< 10;
				*p_meta_entry_first |= meta_area.GetZ()	<< 18;
	
				*p_meta_entry_second = p_meta->GetRot() & 3;
				*p_meta_entry_second |= ((meta_area.GetY() + 16) & 31) << 2;
				
				*p_meta_entry_first = (*p_meta_entry_first);
				*p_meta_entry_second = (*p_meta_entry_second);
				//Ryan("compressed piece: 0x%x 0x%x\n", *p_meta_entry_first, *p_meta_entry_second);
				
				p_meta_entry += 8;
				num_metas_outputted++;
			}
		}

		p_meta_node = p_meta_node->GetNext();
	}

	int buffer_used_size = (uint32) p_meta_entry - (uint32) p_header;
	Dbg_MsgAssert(buffer_used_size <= COMPRESSED_MAP_SIZE, ("compressed map buffer (%d) needs to be bigger than (%d)",COMPRESSED_MAP_SIZE,buffer_used_size));
	p_header->mNumMetas = (num_metas_outputted);	
	
	/* Gap Section */

	// count number of gaps
	CGapManager::GapDescriptor *p_desc_tab[CGapManager::vMAX_GAPS];
	int num_gaps = 0;
	int gap;
	for (gap = 0; gap < CGapManager::vMAX_GAPS; gap++)
	{
		int half = 0;
		CGapManager::GapDescriptor *pDesc = mp_gap_manager->GetGapDescriptor(gap, &half);
		if (pDesc && half == 0 && pDesc->numCompleteHalves == 2)
			p_desc_tab[num_gaps++] = pDesc;
	}
	p_header->mNumGaps = (num_gaps);
	
	// write gaps
	CompressedGap *p_out_gap = (CompressedGap *) p_meta_entry;
	for (gap = 0; gap < num_gaps; gap++)
	{
		p_out_gap->rot = 0;
		for (int half = 0; half < 2; half++)
		{
			p_out_gap->x[half] = p_desc_tab[gap]->loc[half].GetX(); 
			p_out_gap->y[half] = p_desc_tab[gap]->loc[half].GetY() + (MAX_HEIGHT>>1); 
			p_out_gap->z[half] = p_desc_tab[gap]->loc[half].GetZ(); 
			p_out_gap->rot |= p_desc_tab[gap]->rot[half]<<(half*4);
			
			p_out_gap->extent[half] = (uint8) ((p_desc_tab[gap]->leftExtent[half] & 15) << 4);
			p_out_gap->extent[half] |= (uint8) (p_desc_tab[gap]->rightExtent[half] & 15);
			//Ryan("adding gap at (%d,%d,%d)\n", p_out_gap->x[half], p_out_gap->y[half], p_out_gap->z[half]);
		}
		Dbg_MsgAssert(strlen(p_desc_tab[gap]->text) <= 31, ("too many characters in gap name %s", p_desc_tab[gap]->text));
		strcpy(p_out_gap->text, p_desc_tab[gap]->text);
		p_out_gap->score = ((uint16) p_desc_tab[gap]->score);
		
		p_out_gap->mCancelFlags=p_desc_tab[gap]->mCancelFlags;
		
		p_out_gap++;
	}
  
	buffer_used_size = (uint32) p_out_gap - (uint32) p_header;
	Dbg_MsgAssert(buffer_used_size <= COMPRESSED_MAP_SIZE, ("compressed map buffer (%d) needs to be bigger than (%d)",COMPRESSED_MAP_SIZE,buffer_used_size));
	p_header->mSizeInBytes = buffer_used_size;
	
	/* Flag section */

	if (!(m_compressed_map_flags & mIN_SYNC_WITH_PARK) && !(m_compressed_map_flags & mIS_NEWER_THAN_PARK))
		m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags | mNOT_SAVED_LOCAL);
	
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags | (mIN_SYNC_WITH_PARK + mIS_VALID));
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~mIS_NEWER_THAN_PARK);


	p_header->mChecksum = (Crc::GenerateCRCCaseSensitive((const char *) mp_compressed_map_buffer + 4, COMPRESSED_MAP_SIZE-4));
	
	// XXX
	ParkEd("end CParkManager::write_compressed_map_buffer()");	
}


// Create a minimal compressed map buffer
void CParkManager::fake_compressed_map_buffer()
{
	ParkEd("CParkManager::fake_compressed_map_buffer()");	
	
//	Script::CArray *p_save_map_array = Script::GetArray("Ed_Save_Map", Script::ASSERT);
//	uint save_map_array_size = p_save_map_array->GetSize();
	
	setup_default_dimensions();
	
	CompressedMapHeader *p_header = (CompressedMapHeader *) mp_compressed_map_buffer;
	p_header->mVersion = (VERSION);
	p_header->mTheme = (m_theme);
	p_header->mParkSize = (0);
	p_header->mX = m_park_near_bounds.GetX();
	p_header->mZ = m_park_near_bounds.GetZ();
	p_header->mW = m_park_near_bounds.GetW();
	p_header->mL = m_park_near_bounds.GetL();
	
	Ed::CParkEditor *p_editor=Ed::CParkEditor::Instance();
	if (p_editor)
	{
		p_header->mTODScript = p_editor->GetTimeOfDayScript();
	}
	else
	{
		p_header->mTODScript=0;
	}	
	
	p_header->mMaxPlayers=2;
	strcpy(p_header->mParkName, "unnamed park");

	CompressedFloorEntry *p_floor_entry = (CompressedFloorEntry *) (p_header + 1);		
	for (int i = 0; i < FLOOR_MAX_WIDTH; i++)
	{
		for (int j = 0; j < FLOOR_MAX_LENGTH; j++)
		{
			p_floor_entry->mHeight = 0;		// (sint8) m_floor_height_map[i][j].mHeight;
			p_floor_entry++;
		}
	}

	uint8 *p_meta_entry = (uint8 *) p_floor_entry;
	int num_metas_outputted = 0;
	int buffer_used_size = (uint32) p_meta_entry - (uint32) p_header;
	p_header->mNumMetas = num_metas_outputted;	
	p_header->mChecksum = Crc::GenerateCRCCaseSensitive((const char *) mp_compressed_map_buffer + 4, buffer_used_size - 4);
	int num_gaps = 0;
	p_header->mNumGaps = num_gaps;
	
	// write gaps
	CompressedGap *p_out_gap = (CompressedGap *) p_meta_entry;
	buffer_used_size = (uint32) p_out_gap - (uint32) p_header;
	Dbg_MsgAssert(buffer_used_size <= COMPRESSED_MAP_SIZE, ("compressed map buffer (%d) needs to be bigger than (%d)",COMPRESSED_MAP_SIZE,buffer_used_size));
	p_header->mSizeInBytes = buffer_used_size;
	
	/* Flag section */

	if (!(m_compressed_map_flags & mIN_SYNC_WITH_PARK) && !(m_compressed_map_flags & mIS_NEWER_THAN_PARK))
		m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags | mNOT_SAVED_LOCAL);
	
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags | (mIN_SYNC_WITH_PARK + mIS_VALID));
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~mIS_NEWER_THAN_PARK);
	
	// XXX
	ParkEd("end CParkManager::fake_compressed_map_buffer()");	
}




void CParkManager::read_from_compressed_map_buffer()
{
	Dbg_Assert(m_state_on);
	// XXX
	ParkEd("CParkManager::read_from_compressed_map_buffer()");	
	
	Script::CArray *p_save_map_array = Script::GetArray("Ed_Save_Map", Script::ASSERT);
	uint32 dead_piece_checksum = Script::GenerateCRC("Sk4Ed_Dead");
	
	Dbg_MsgAssert(m_compressed_map_flags & mIS_VALID, ("compressed map is invalid"));
	
	CompressedMapHeader *p_header = (CompressedMapHeader *) mp_compressed_map_buffer;
	// make sure latest version
	if (p_header->mVersion != VERSION)
	{
		read_from_compressed_map_buffer_old();
		GetGenerator()->SetMaxPlayers(1);
		return;
	}

	m_park_near_bounds[X] = p_header->mX;
	m_park_near_bounds[Z] = p_header->mZ;
	m_park_near_bounds[W] = p_header->mW;
	m_park_near_bounds[L] = p_header->mL;
	m_theme = p_header->mTheme;
	Ed::CParkEditor::Instance()->SetTimeOfDayScript(p_header->mTODScript);
	#ifdef __PLAT_NGC__
	p_header->mMaxPlayers=2;
	#endif
	GetGenerator()->SetMaxPlayers(p_header->mMaxPlayers);
	// SG: Was setting to the incorrect number (8 when it should have been 2). I think
	// it is redundant anyway as max players is set elsewhere already
	//GameNet::Manager::Instance()->SetMaxPlayers(p_header->mMaxPlayers);
	
	CompressedFloorEntry *p_floor_entry = (CompressedFloorEntry *) (p_header + 1);		
	for (int i = 0; i < FLOOR_MAX_WIDTH; i++)
	{
		for (int j = 0; j < FLOOR_MAX_LENGTH; j++)
		{
			m_floor_height_map[i][j].mHeight = (int) p_floor_entry->mHeight;
			m_floor_height_map[i][j].mEnclosedFloor = 0;
			p_floor_entry++;
		}
	}

	uint8 *p_meta_entry = (uint8 *) p_floor_entry;
	Dbg_Printf( "****** NUM METAS WHILE READING FROM BUFFER : %d\n", p_header->mNumMetas );
	for (int i = 0; i < p_header->mNumMetas; i++)
	{
		uint32 *p_meta_entry_first = (uint32 *) p_meta_entry;
		uint32 *p_meta_entry_second = (uint32 *) (p_meta_entry + 4);
		uint32 meta1 = *p_meta_entry_first;
		uint32 meta2 = *p_meta_entry_second;
		
		//Ryan("compressed piece: 0x%x 0x%x\n", *p_meta_entry_first, *p_meta_entry_second);
		
		uint32 metapiece_index = meta1 & 1023;
		uint32 metapiece_checksum = p_save_map_array->GetChecksum(metapiece_index);

		if (metapiece_checksum != dead_piece_checksum)
		{
			CAbstractMetaPiece *p_source_meta = GetAbstractMeta(metapiece_checksum);
			Dbg_MsgAssert(p_source_meta, ("couldn't find abstract meta %s", Script::FindChecksumName(metapiece_checksum)));
			#ifdef __PLAT_NGC__
			if (!(p_source_meta->m_flags & CMetaPiece::mNOT_ON_GC))			
			#endif
			{			
				CConcreteMetaPiece *p_meta = CreateConcreteMeta(p_source_meta);
				GridDims pos;
				pos[X] = (meta1 >> 10) & 255;
				pos[Z] = (meta1 >> 18) & 255;
				pos[Y] = ((meta2 >> 2) & 31) - 16;
				int rot = meta2 & 3;
				p_meta->SetRot(Mth::ERot90(rot));
				AddMetaPieceToPark(pos, p_meta);
			}
		}
		
		p_meta_entry += 8;
	}	
	
	/* Gap section */
	
	CompressedGap *p_in_gap = (CompressedGap *) p_meta_entry;
	for (int g = 0; g < p_header->mNumGaps; g++)
	{
		CGapManager::GapDescriptor desc;
		
		for (int half = 0; half < 2; half++)
		{
			desc.loc[half][X] = p_in_gap->x[half]; 
			desc.loc[half][Y] = p_in_gap->y[half] - (MAX_HEIGHT>>1); 
			desc.loc[half][Z] = p_in_gap->z[half]; 
			desc.rot[half] = (p_in_gap->rot>>(half*4)) & 15;
			
			desc.leftExtent[half] = (p_in_gap->extent[half]>>4) & 15;
			desc.rightExtent[half] = (p_in_gap->extent[half]) & 15;
			//Ryan("adding gap at (%d,%d,%d)\n", p_in_gap->x[half], p_in_gap->y[half], p_in_gap->z[half]);
		}
		Dbg_Assert(strlen(p_in_gap->text) < 32);
		strcpy(desc.text, p_in_gap->text);
		desc.score = (int) p_in_gap->score;
		desc.numCompleteHalves = 2;
		desc.mCancelFlags=p_in_gap->mCancelFlags;
		
		AddGap(desc);
		
		p_in_gap++;
	}
	
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags | mIN_SYNC_WITH_PARK);
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~mIS_NEWER_THAN_PARK);
	
	// XXX
	ParkEd("end CParkManager::read_from_compressed_map_buffer()");	
}



void CParkManager::read_from_compressed_map_buffer_old()
{
	Dbg_Assert(m_state_on);
	// XXX
	ParkEd("CParkManager::read_from_compressed_map_buffer()");	
	
	Script::CArray *p_save_map_array = Script::GetArray("Ed_Save_Map", Script::ASSERT);
	uint32 dead_piece_checksum = Script::GenerateCRC("Sk4Ed_Dead");
	
	Dbg_MsgAssert(m_compressed_map_flags & mIS_VALID, ("compressed map is invalid"));
	
	CompressedMapHeader *p_header = (CompressedMapHeader *) mp_compressed_map_buffer;

	// make sure proper version
	Dbg_MsgAssert((p_header->mVersion) == VERSION-1, ("can't read park with this version #: %d", (p_header->mVersion)));
	
	// XXX
//	for (int zzz = 0; zzz < 16; zzz++)
//		printf("OUT OF DATE PARK FORMAT -- SAVE NOW. OUT OF DATE PARK FORMAT -- SAVE NOW. OUT OF DATE PARK FORMAT -- SAVE NOW.\n");

	m_park_near_bounds[X] = p_header->mX;
	m_park_near_bounds[Z] = p_header->mZ;
	m_park_near_bounds[W] = p_header->mW;
	m_park_near_bounds[L] = p_header->mL;
	m_theme = (p_header->mTheme);
	Ed::CParkEditor::Instance()->SetTimeOfDayScript(p_header->mTODScript);
	
	GetGenerator()->SetMaxPlayers(p_header->mMaxPlayers);
	// SG: Was setting to the incorrect number (8 when it should have been 2). I think
	// it is redundant anyway as max players is set elsewhere already
	//GameNet::Manager::Instance()->SetMaxPlayers(p_header->mMaxPlayers);
	
	CompressedFloorEntry *p_floor_entry = (CompressedFloorEntry *) (p_header + 1);		
	for (int i = 0; i < FLOOR_MAX_WIDTH; i++)
	{
		for (int j = 0; j < FLOOR_MAX_LENGTH; j++)
		{
			m_floor_height_map[i][j].mHeight = (int) p_floor_entry->mHeight;
			m_floor_height_map[i][j].mEnclosedFloor = 0;
			p_floor_entry++;
		}
	}

	uint8 *p_meta_entry = (uint8 *) p_floor_entry;
	Dbg_Printf( "****** NUM METAS WHILE READING FROM BUFFER : %d\n", p_header->mNumMetas );
	for (int i = 0; i < (p_header->mNumMetas); i++)
	{
		uint32 *p_meta_entry_first = (uint32 *) p_meta_entry;
		uint32 *p_meta_entry_second = (uint32 *) (p_meta_entry + 4);
		uint32 meta1 = (*p_meta_entry_first);
		uint32 meta2 = (*p_meta_entry_second);
		
		//Ryan("compressed piece: 0x%x 0x%x\n", *p_meta_entry_first, *p_meta_entry_second);
		
		uint32 metapiece_index = meta1 & 1023;
		uint32 metapiece_checksum = p_save_map_array->GetChecksum(metapiece_index);

		if (metapiece_checksum != dead_piece_checksum)
		{
			CAbstractMetaPiece *p_source_meta = GetAbstractMeta(metapiece_checksum);
			Dbg_MsgAssert(p_source_meta, ("couldn't find abstract meta %s", Script::FindChecksumName(metapiece_checksum)));
			#ifdef __PLAT_NGC__
			if (!(p_source_meta->m_flags & CMetaPiece::mNOT_ON_GC))			
			#endif
			{			
				CConcreteMetaPiece *p_meta = CreateConcreteMeta(p_source_meta);
				GridDims pos;
				pos[X] = (meta1 >> 10) & 255;
				pos[Z] = (meta1 >> 18) & 255;
				pos[Y] = ((meta2 >> 2) & 31) - 16;
				int rot = meta2 & 3;
				p_meta->SetRot(Mth::ERot90(rot));
				AddMetaPieceToPark(pos, p_meta);
			}
		}
		
		p_meta_entry += 8;
	}	
	
	// Gap section
	
	CompressedGapOld *p_in_gap = (CompressedGapOld *) p_meta_entry;
	for (int g = 0; g < (p_header->mNumGaps); g++)
	{
		CGapManager::GapDescriptor desc;
		
		for (int half = 0; half < 2; half++)
		{
			desc.loc[half][X] = p_in_gap->x[half]; 
			desc.loc[half][Y] = p_in_gap->y[half] - (MAX_HEIGHT>>1); 
			desc.loc[half][Z] = p_in_gap->z[half]; 
			desc.rot[half] = (p_in_gap->rot>>(half*4)) & 15;
			
			desc.leftExtent[half] = (p_in_gap->extent[half]>>4) & 15;
			desc.rightExtent[half] = (p_in_gap->extent[half]) & 15;
			//Ryan("adding gap at (%d,%d,%d)\n", p_in_gap->x[half], p_in_gap->y[half], p_in_gap->z[half]);
		}
		Dbg_Assert(strlen(p_in_gap->text) < 32);
		strcpy(desc.text, p_in_gap->text);
		desc.score = (int) (p_in_gap->score);
		desc.numCompleteHalves = 2;
		
		AddGap(desc);
		
		p_in_gap++;
	}
	
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags | mIN_SYNC_WITH_PARK);
	m_compressed_map_flags = ECompressedMapFlags(m_compressed_map_flags & ~mIS_NEWER_THAN_PARK);
	
	// XXX
	ParkEd("end CParkManager::read_from_compressed_map_buffer_old()");	
}

void	CParkManager::setup_default_dimensions()
{
	int default_inner_dim = Script::GetInt("ed_default_inner_dim", Script::ASSERT);
	Dbg_Assert(default_inner_dim <= MAX_WIDTH && default_inner_dim <= MAX_LENGTH);

	m_park_far_bounds.SetXYZ(1, -4, 1);
	m_park_far_bounds.SetWHL(MAX_WIDTH, 8, MAX_LENGTH);
	m_park_near_bounds.SetXYZ(1 + (MAX_WIDTH - default_inner_dim) / 2, -4, 1 + (MAX_LENGTH - default_inner_dim) / 2);
	m_park_near_bounds.SetWHL(default_inner_dim, 8, default_inner_dim);
}



void CParkManager::setup_road_mask()
{
	/*
		The first four entries in m_road_mask_tab will be set up in RebuildInnerShell()
	*/
	
	Script::CArray *p_road_mask_array = Script::GetArray("Ed_Roadmask", Script::ASSERT);
	m_road_mask_tab_size = p_road_mask_array->GetSize();
	for (uint i = 4; i < 4 + m_road_mask_tab_size; i++)
	{
		Dbg_Assert(i < NUM_ROAD_PIECES);
		Script::CArray *p_entry = p_road_mask_array->GetArray(i);

		m_road_mask_tab[i][X] = p_entry->GetInt(0);
		m_road_mask_tab[i][Z] = p_entry->GetInt(1);
		m_road_mask_tab[i][W] = p_entry->GetInt(2);
		m_road_mask_tab[i][L] = p_entry->GetInt(3);
		m_road_mask_tab[i][Y] = 0;
		m_road_mask_tab[i].MakeInfinitelyHigh();
	}

	m_road_mask_tab_size += 4;
}


void CParkManager::setup_lighting()
{
	
	
	Script::CArray *pThemeArray = Script::GetArray("Editor_Light_Info");
//	Script::CArray *pSizeArray = pThemeArray->GetArray(m_currentThemeIndex);
//	Script::CScriptStructure *pLightInfo = pSizeArray->GetStructure(m_currentShellSizeIndex);
	Script::CArray *pSizeArray = pThemeArray->GetArray(0);
	Script::CScriptStructure *pLightInfo = pSizeArray->GetStructure(0);

	float amb_const;
	pLightInfo->GetFloat("ambient_const", &amb_const, true);
	Mth::Vector ambient_rgb;
	pLightInfo->GetVector("ambient_rgb", &ambient_rgb, true);
	float falloff_const;
	pLightInfo->GetFloat("falloff_const", &falloff_const, true);
	Mth::Vector falloff_rgb;
	pLightInfo->GetVector("falloff_rgb", &falloff_rgb, true);
	float cursor_ambience;
	pLightInfo->GetFloat("cursor_ambience", &cursor_ambience, true);
	
	
	Script::CArray *pLightArray;
	pLightInfo->GetArray("pos", &pLightArray, true);

	mp_generator->SetLightProps(pLightArray->GetSize(), 
							amb_const * ambient_rgb.GetX(), amb_const * ambient_rgb.GetY(), amb_const * ambient_rgb.GetZ(), 
							falloff_const * falloff_rgb.GetX(), falloff_const * falloff_rgb.GetY(), falloff_const * falloff_rgb.GetZ(),
							cursor_ambience);

	for (int i = 0; i < (int)pLightArray->GetSize(); i++)
	{
		Script::CVector *pInPos = pLightArray->GetVector(i);
		Mth::Vector outPos;
		outPos.Set(pInPos->mX * CParkGenerator::CELL_WIDTH,
				   pInPos->mY * CParkGenerator::CELL_HEIGHT,
				   pInPos->mZ * CParkGenerator::CELL_LENGTH);
		mp_generator->SetLight(outPos, i);
	}
}




}
