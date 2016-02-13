#include <core/defines.h>
#include <gel/mainloop.h>
#include <gel/objtrack.h>
#include <gel/event.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/utils.h>
#include <sk/ParkEditor2/ParkEd.h>
#include <sk/scripting/cfuncs.h>
#include <sk/modules/FrontEnd/FrontEnd.h>
#include <gfx/nxviewman.h>
#include <gfx/2D/ScreenElemMan.h>
#include <gfx/2D/TextElement.h>

#include <sk/ParkEditor2/clipboard.h>

#include <sk/modules/skate/skate.h>
#include <sk/components/RailEditorComponent.h>
#include <gfx/nx.h>

DefinePoolableClass(Ed::CClipboardEntry)
DefinePoolableClass(Ed::CClipboard)

namespace Ed
{

static void s_rotate_map_vector(int *p_x, int *p_z, Mth::ERot90 rotation)
{
	int in_x=*p_x;
	int in_z=*p_z;
	switch (rotation)
	{
		case Mth::ROT_0:
			break;
		case Mth::ROT_90:
			*p_x=in_z;
			*p_z=-in_x;
			break;
		case Mth::ROT_180:
			*p_x=-in_x;
			*p_z=-in_z;
			break;
		case Mth::ROT_270:
			*p_x=-in_z;
			*p_z=in_x;
			break;
		default:
			Dbg_MsgAssert(0, ("Wacky rotation"));
			break;
	}		
}

CClipboardEntry::CClipboardEntry()
{
	mpMeta=NULL;
	mpConcreteMeta=NULL;
	mX=mY=mZ=0;
	mWidth=0;
	mLength=0;
	m_rot=Mth::ROT_0;
	mpNext=NULL;
}

CClipboardEntry::~CClipboardEntry()
{
	DestroyMeta();
}

void CClipboardEntry::DestroyMeta()
{
	if (mpConcreteMeta)
	{
		mpConcreteMeta->MarkUnlocked();
		CParkManager::sInstance()->DestroyConcreteMeta(mpConcreteMeta, CParkManager::mDONT_DESTROY_PIECES_ABOVE);
		mpConcreteMeta=NULL;
	}
}

// This function takes the vector from the centre to the piece, rotates it according to 
// the cursor's current rotation, then adds that vector to the cursor's position and places the result in
// p_mapCoords
void CClipboardEntry::CalculateMapCoords(GridDims *p_mapCoords, uint8 centre_x, uint8 centre_z, CCursor *p_cursor)
{
	GridDims meta_area;
	if (mpMeta)
	{
		meta_area=mpMeta->GetArea();
	}
	else
	{
		// If there is no meta then the clipboard entry is defining a height for one cell, so
		// just set the w and l to 1.
		meta_area.SetWHL(1,1,1);
	}
		
	Mth::ERot90 total_rotation=Mth::ERot90((p_cursor->GetRotation()+m_rot)&3);
	switch ( total_rotation )
	{
		case Mth::ROT_0:
		case Mth::ROT_180:
			p_mapCoords->SetWHL(meta_area.GetW(),1,meta_area.GetL());
			break;
		case Mth::ROT_90:
		case Mth::ROT_270:
			p_mapCoords->SetWHL(meta_area.GetL(),1,meta_area.GetW());
			break;
		default:
			Dbg_MsgAssert(0, ("Wacky rotation"));
			break;
	}		
	
	// Get the vector from the centre of the selection area to the piece.
	int vector_x=mX-centre_x;
	int vector_z=mZ-centre_z;
	
	// Rotate the vector according to the cursor's current rotation
	s_rotate_map_vector(&vector_x, &vector_z, p_cursor->GetRotation());

	// A frig is needed after rotation because the piece always appears to be
	// displayed with the coords referring to the top-left corner, even when the
	// piece is rotated.
	switch ( p_cursor->GetRotation() )
	{
		case Mth::ROT_0:
			break;
		case Mth::ROT_90:
			vector_z-=mWidth-1;
			break;
		case Mth::ROT_180:
			vector_x-=mWidth-1;
			vector_z-=mLength-1;
			break;	
		case Mth::ROT_270:
			vector_x-=mLength-1;
			break;
		default:
			Dbg_MsgAssert(0, ("Wacky rotation"));
			break;
	}		

	// Get the new map coords of the meta such that it is positioned relative to the cursor.
	GridDims cursor_position=p_cursor->GetPosition();
	p_mapCoords->SetXYZ(cursor_position.GetX()+vector_x, mY, cursor_position.GetZ()+vector_z);
}

bool CClipboardEntry::CanPaste(uint8 centre_x, uint8 centre_z, CCursor *p_cursor)
{
	GridDims map_coords;
	CalculateMapCoords(&map_coords,centre_x,centre_z,p_cursor);
	// Make sure the y is set to zero because the GetMetaPiecesAt function that gets called in
	// CanPlacePiecesIn will think there is something in the cell if the y is -1
	map_coords.SetXYZ(map_coords.GetX(),0,map_coords.GetZ());
	
	CParkManager *p_manager=CParkManager::sInstance();
	return p_manager->CanPlacePiecesIn(map_coords, true); // The true means ignore floor height problems
}

// centre_x,centre_z are the map coords of the centre of the original selection area.
// The coords of the entry (mX,mZ) are also in map coords.
void CClipboardEntry::Paste(uint8 centre_x, int raiseAmount, uint8 centre_z, CCursor *p_cursor)
{
	if (mpMeta && mpMeta->IsRiser())
	{
		return;
	}

	GridDims map_coords;
	CalculateMapCoords(&map_coords,centre_x,centre_z,p_cursor);
	
	int new_y=mY+raiseAmount;
	if (new_y >= CParkManager::MAX_HEIGHT)
	{
		new_y=CParkManager::MAX_HEIGHT-1;
	}

	map_coords.SetXYZ(map_coords.GetX(),new_y,map_coords.GetZ());
		
	CParkManager *p_manager=CParkManager::sInstance();
	
	// Adjust the floor height to match that of the piece
	int count=0;	
	while (true)
	{
		bool uniform_height=false;
		int current_height = p_manager->GetFloorHeight(map_coords, &uniform_height);
		
		if (current_height > new_y)
		{
			break;
		}	
		
		if (count>40)
		{
			Dbg_MsgAssert(0,("Too many recursions of ChangeFloorHeight in CClipboardEntry::Paste"));
			break;
		}	
			
		if (uniform_height && current_height==new_y)
		{
			break;
		}
		
		p_manager->ChangeFloorHeight(map_coords,1); // 1 is the step amount, 1 upwards
		++count;
	}		

	if (mpMeta)
	{
		// Place the piece	
		CConcreteMetaPiece *p_concrete = p_manager->CreateConcreteMeta(mpMeta);
		p_concrete->SetRot(Mth::ERot90((m_rot+p_cursor->GetRotation())&3));
		p_manager->AddMetaPieceToPark(map_coords, p_concrete);
	}	
}

int CClipboardEntry::GetFloorHeight(uint8 centre_x, uint8 centre_z, CCursor *p_cursor)
{
	GridDims map_coords;
	CalculateMapCoords(&map_coords,centre_x,centre_z,p_cursor);
	
	bool uniform_height;
	return CParkManager::sInstance()->GetFloorHeight(map_coords, &uniform_height);
}

void CClipboardEntry::HighlightIntersectingMetas(uint8 centre_x, uint8 centre_z, CCursor *p_cursor, bool on)
{
	GridDims map_coords;
	CalculateMapCoords(&map_coords,centre_x,centre_z,p_cursor);
	// Make sure the y is set to zero because the GetMetaPiecesAt function that gets called in
	// CanPlacePiecesIn will think there is something in the cell if the y is -1
	map_coords.SetXYZ(map_coords.GetX(),0,map_coords.GetZ());
	
	CParkManager *p_manager=CParkManager::sInstance();
	
	CMapListTemp metas_at_pos = p_manager->GetAreaMetaPieces(map_coords);
	if (metas_at_pos.IsEmpty())
	{
		return;
	}
	
	// Iterate over the list of metapieces we just found, and highlight each one
	CMapListNode *p_entry = metas_at_pos.GetList();
	while(p_entry)
	{
		CConcreteMetaPiece *p_meta=p_entry->GetConcreteMeta();
		if (!p_meta->IsRiser())
		{
			p_meta->Highlight(on);		
		}
			
		p_entry = p_entry->GetNext();
	}
}

// This gets called when the CCursor is in paste mode, and needs to display the clipboard contents at
// the cursor position.
// pos and rot are the world position and rotation in degrees of the cursor.
// They may not be cell aligned, and the rotation may not be a multiple of 90 degrees.
// This function will position the meta such that the centre of the clipboard selection is at pos.
void CClipboardEntry::ShowMeta(uint8 centre_x, int raiseAmount, uint8 centre_z, Mth::Vector pos, float rot)
{
	if (!mpMeta)
	{
		// No meta to display, meaning the clipboard entry is defining a cell height
		return;
	}

	// Get the vector (in cell units) from the centre of the selection area to the piece.
	float vector_x=mX-centre_x;
	float vector_z=mZ-centre_z;

	// Calculate the vector, in world coords, from the centre of the selection to the centre of the piece.	
	Mth::Vector p(0.0f,0.0f,0.0f);
	p[X]=vector_x*CParkGenerator::CELL_WIDTH+(mWidth-1)*CParkGenerator::CELL_WIDTH/2.0f;
	p[Z]=vector_z*CParkGenerator::CELL_LENGTH+(mLength-1)*CParkGenerator::CELL_LENGTH/2.0f;

	// Rotate by rot degrees	
	float rad=rot*3.141592654f/180.0f;
	float s=sinf(rad);
	float c=cosf(rad);

	Mth::Vector rotated;
	rotated[X]=c*p[X]+s*p[Z];
	rotated[Y]=0.0f;
	rotated[Z]=c*p[Z]-s*p[X];
	
	// Add the passed pos to get the final world position of the piece.
	Mth::Vector world_pos=pos+rotated;

	// Add in the cell height
	world_pos[Y]+=mY*CParkGenerator::CELL_HEIGHT;
	if (mpMeta->IsRiser())
	{
		world_pos[Y]-=CParkGenerator::CELL_HEIGHT;
	}	
	
	// Create the concrete meta if it does not exist already.
	if (!mpConcreteMeta)
	{
		Dbg_MsgAssert(mpMeta,("NULL mpMeta ?"));
		mpConcreteMeta=CParkManager::sInstance()->CreateConcreteMeta(mpMeta,true);
		mpConcreteMeta->MarkLocked();
	}	
	
	// Position the piece, adding in the pieces local rotation value.
	mpConcreteMeta->SetSoftRot(world_pos,rot+90.0f*m_rot);
}

bool CClipboardEntry::CreateGapFillerPieces()
{
	if (!mpMeta)
	{
		//printf("y=%d\n",mY);
		return true;
	}

	//printf("y=%d Meta=%s\n",mY,Script::FindChecksumName(mpMeta->GetNameChecksum()));

	switch (mpMeta->GetNameChecksum())
	{
	case CRCC(0x66a8fedb,"floor_block1"):
	case CRCC(0xffa1af61,"floor_block2"):
	case CRCC(0x88a69ff7,"floor_block3"):
	case CRCC(0x16c20a54,"floor_block4"):
	case CRCC(0x61c53ac2,"floor_block5"):
	case CRCC(0xf8cc6b78,"floor_block6"):
	case CRCC(0x8fcb5bee,"floor_block7"):
	case CRCC(0x1f74467f,"floor_block8"):
		break;
	default:
		return true;
		break;
	}		

	//printf("y=%d Meta=%s\n",mY,Script::FindChecksumName(mpMeta->GetNameChecksum()));

	bool ran_out_of_memory=false;
	CClipboardEntry *p_new_list=NULL;
	CClipboardEntry *p_last_in_list=NULL;
	for (int y=mY; y>0; --y)
	{
		if (Mem::CPoolable<CClipboardEntry>::SGetNumUsedItems()==
			Mem::CPoolable<CClipboardEntry>::SGetTotalItems())
		{
			// Not enough space to create a new CClipboardEntry !
			ran_out_of_memory=true;
			break;
		}	
				
		CClipboardEntry *p_new_entry=new CClipboardEntry;
		if (!p_last_in_list)
		{
			p_last_in_list=p_new_entry;
		}	

		if (y==mY)
		{
			p_new_entry->mpMeta = CParkManager::sInstance()->GetAbstractMeta(CRCD(0x2c5b0277,"floor_wall_block3"));
		}
		else
		{
			p_new_entry->mpMeta = CParkManager::sInstance()->GetAbstractMeta(CRCD(0x72372c50,"wall_block1"));
		}
			
		p_new_entry->mX=mX;
		p_new_entry->mY=y;
		p_new_entry->mZ=mZ;
		p_new_entry->mWidth=1;
		p_new_entry->mLength=1;
		p_new_entry->m_rot=m_rot;
		
		// Insert into the list.
		p_new_entry->mpNext=p_new_list;
		p_new_list=p_new_entry;
	}
	
	if (p_new_list)
	{
		Dbg_MsgAssert(p_last_in_list,("Eh?"));
		p_last_in_list->mpNext=mpNext;
		mpNext=p_new_list;
	}
		
	if (ran_out_of_memory)
	{
		return false;
	}
		
	return true;
}
	
CClipboard::CClipboard()
{
	mp_entries=NULL;
	m_min_x=m_min_z=255;
	m_max_x=m_max_z=0;
	
	mpNext=NULL;
	m_area.SetXYZ(0,0,0);
	m_area.SetWHL(0,0,0);
	
	mp_rails=NULL;
}

CClipboard::~CClipboard()
{
	delete_entries();
}

void CClipboard::delete_entries()
{
	CClipboardEntry *p_entry=mp_entries;
	while (p_entry)
	{
		CClipboardEntry *p_next=p_entry->mpNext;
		delete p_entry;
		p_entry=p_next;
	}	
	mp_entries=NULL;
	
	m_min_x=m_min_z=255;
	m_max_x=m_max_z=0;
	
	m_area.SetXYZ(0,0,0);
	m_area.SetWHL(0,0,0);
	
	// Call UpdateSuperSectors to refresh the collision before deleting the rails otherwise the 
	// collision code will assert when the rail sectors are deleted. (TT6874)
	// UpdateSuperSectors isn;t normally called on the rails displayed on the cursor because
	// they don't need collision.
	Nx::CScene *p_cloned_scene=Ed::CParkManager::sInstance()->GetGenerator()->GetClonedScene();
	p_cloned_scene->UpdateSuperSectors();

	Obj::CEditedRail *p_rail=mp_rails;
	while (p_rail)
	{
		Obj::CEditedRail *p_next=p_rail->mpNext;
		delete p_rail;
		p_rail=p_next;
	}
	mp_rails=NULL;
}

void CClipboard::update_extents(uint8 x, uint8 z)
{
	if (x < m_min_x)
	{
		m_min_x=x;
	}
	if (x > m_max_x)
	{
		m_max_x=x;
	}
	if (z < m_min_z)
	{
		m_min_z=z;
	}
	if (z > m_max_z)
	{
		m_max_z=z;
	}
}

// Returns false if it fails due to running out of space on the CClipboardEntry pool
bool CClipboard::AddMeta(CConcreteMetaPiece *p_meta, int raiseAmount)
{
	Dbg_MsgAssert(p_meta,("NULL p_meta"));

	GridDims dims=p_meta->GetArea();
	dims.SetXYZ(dims.GetX(),dims.GetY()+raiseAmount,dims.GetZ());
	//printf("AddMeta y=%d raiseAmount=%d, \t%s Riser=%d\n",dims.GetY(),raiseAmount,Script::FindChecksumName(p_meta->GetNameChecksum()),p_meta->IsRiser());
	
	if (p_meta->IsRiser())
	{
		dims.SetXYZ(dims.GetX(),dims.GetY()+1,dims.GetZ());
		
		//printf("Riser: %d,%d  %d\n",dims.GetX(),dims.GetZ(),dims.GetY());
		
		if (dims.GetY())
		{
			if (!AddHeight(dims.GetX(),dims.GetY(),dims.GetZ()))
			{
				return false;
			}	
		}	
		// Zero height riser pieces (ie, floor pieces) still get added so that they
		// are displayed on the cursor.
	}	

	CAbstractMetaPiece *p_abstract_meta=CParkManager::sInstance()->GetAbstractMeta(p_meta->GetNameChecksum());
	
	// Don't add meta if it is in the list already
	CClipboardEntry *p_entry=mp_entries;
	while (p_entry)
	{
		if (p_entry->mpMeta==p_abstract_meta &&
			p_entry->mX==dims.GetX() &&
			p_entry->mY==dims.GetY() &&
			p_entry->mZ==dims.GetZ() &&
			p_entry->mWidth==dims.GetW() &&
			p_entry->mLength==dims.GetL() &&
			p_entry->m_rot==p_meta->GetRot())
		{
			return true;
		}
		p_entry=p_entry->mpNext;
	}	

	if (Mem::CPoolable<CClipboardEntry>::SGetNumUsedItems()==
		Mem::CPoolable<CClipboardEntry>::SGetTotalItems())
	{
		// Not enough space to create a new CClipboardEntry !
		return false;
	}	
			
	CClipboardEntry *p_new_entry=new CClipboardEntry;

	// Store a pointer to the abstract meta so that a copy can be made when the time comes to paste.	
	// Mustn't store a pointer to the concrete meta because the user may delete that piece later.
	p_new_entry->mpMeta = p_abstract_meta;

	// Store the grid coords of the concrete meta because the abstract meta's x,y,z will be zero.
	p_new_entry->mX=dims.GetX();
	p_new_entry->mY=dims.GetY();
	p_new_entry->mZ=dims.GetZ();
	// Also store the width and length, needed for calculating the world pos of the centre when
	// displaying the piece on the cursor.
	// Note that the width and height will already take into account any rotation that the concrete meta has.
	p_new_entry->mWidth=dims.GetW();
	p_new_entry->mLength=dims.GetL();

	// Update the min & max x & z for the clipboard selection for calculation of the centre later.
	update_extents(dims.GetX(),					dims.GetZ());
	update_extents(dims.GetX()+dims.GetW()-1,	dims.GetZ()+dims.GetL()-1);
	
	p_new_entry->m_rot=p_meta->GetRot();
	
	// Insert into the list.
	p_new_entry->mpNext=mp_entries;
	mp_entries=p_new_entry;
	return true;
}

// Returns false if it fails due to running out of space on the CClipboardEntry pool
bool CClipboard::AddHeight(int x, int y, int z)
{
	// If there is a piece covering this position already then do not add
	// a new height, to prevent unnecessary column slides.
	CClipboardEntry *p_entry=mp_entries;
	while (p_entry)
	{
		if (p_entry->mpMeta && !p_entry->mpMeta->IsRiser() &&
			p_entry->mX <= x && x < p_entry->mX+p_entry->mWidth &&
			p_entry->mZ <= z && z < p_entry->mZ+p_entry->mLength)
		{
			return true;
		}
		p_entry=p_entry->mpNext;
	}	
	
	// If there is a height defined for x,z already then just update it's y
	// if the passed y is bigger.
	p_entry=mp_entries;
	while (p_entry)
	{
		if (p_entry->mpMeta==NULL &&
			p_entry->mX <= x && x < p_entry->mX+p_entry->mWidth &&
			p_entry->mZ <= z && z < p_entry->mZ+p_entry->mLength)
		{
			if (y >= p_entry->mY)
			{
				p_entry->mY=y;
			}
			return true;
		}
		p_entry=p_entry->mpNext;
	}	
	
	if (Mem::CPoolable<CClipboardEntry>::SGetNumUsedItems()==
		Mem::CPoolable<CClipboardEntry>::SGetTotalItems())
	{
		// Not enough space to create a new CClipboardEntry !
		return false;
	}	

	CClipboardEntry *p_new_entry=new CClipboardEntry;

	p_new_entry->mpMeta = NULL;

	p_new_entry->mX=x;
	p_new_entry->mY=y;
	p_new_entry->mZ=z;
	p_new_entry->mWidth=1;
	p_new_entry->mLength=1;

	// Update the min & max x & z for the clipboard selection for calculation of the centre later.
	update_extents(x,z);
	
	// Insert into the list.
	p_new_entry->mpNext=mp_entries;
	mp_entries=p_new_entry;
	return true;
}

// Creates clipboard entries for all the things in the passed area.
// A clipboard entry will be created if there is a piece in a cell, or if
// the cell has a non-zero height.
// Returns false if it fails due to running out of space on the CClipboardEntry pool.
// If that happens, it will delete any CClipboardEntry's that it already allocated.
bool CClipboard::CopySelectionToClipboard(GridDims area)
{
	m_area=area;
	
	CParkManager *p_manager=CParkManager::sInstance();

	// First, figure out how much the selection needs to be raised up so that
	// all the pieces are above ground level.
	int most_negative_y=0;	
	for (int x = area.GetX(); x < area.GetX() + area.GetW(); x++)
	{
		for (int z = area.GetZ(); z < area.GetZ() + area.GetL(); z++)
		{
			CMapListNode *p_entry=p_manager->GetMapListNode(x,z);
			while (p_entry)
			{
				CConcreteMetaPiece *p_meta=p_entry->GetConcreteMeta();
				
				//printf("Meta name = %s\n",Script::FindChecksumName(p_meta->GetNameChecksum()));
				int y=p_meta->GetArea().GetY();
				switch (p_meta->GetNameChecksum())
				{
				case CRCC(0x66a8fedb,"floor_block1"):
				case CRCC(0xffa1af61,"floor_block2"):
				case CRCC(0x88a69ff7,"floor_block3"):
				case CRCC(0x16c20a54,"floor_block4"):
				case CRCC(0x61c53ac2,"floor_block5"):
				case CRCC(0xf8cc6b78,"floor_block6"):
				case CRCC(0x8fcb5bee,"floor_block7"):
				case CRCC(0x1f74467f,"floor_block8"):
					// With floor blocks, the ground position is 1 unit higher up than
					// their stored y, because their stored y is the y of their base.
					++y;
					break;
				default:
					break;
				}		
				if (y < most_negative_y)
				{
					most_negative_y=y;
				}

				//printf("y=%d  %s\n",p_meta->GetArea().GetY(),Script::FindChecksumName(p_meta->GetNameChecksum()));
				
				p_entry = p_entry->GetNext();
			}
		}
	}
	
	
	int raise_amount=-most_negative_y;
	
	
	for (int x = area.GetX(); x < area.GetX() + area.GetW(); x++)
	{
		for (int z = area.GetZ(); z < area.GetZ() + area.GetL(); z++)
		{
			CMapListNode *p_entry=p_manager->GetMapListNode(x,z);
			while (p_entry)
			{
				CConcreteMetaPiece *p_meta=p_entry->GetConcreteMeta();
				if (p_meta->IsRestartOrFlag())
				{
					// Don't copy restarts or flags (TT7793)
				}
				else if (!p_meta->IsCompletelyWithinArea(area))
				{
				}
				else
				{
					if (!AddMeta(p_meta,raise_amount))
					{
						// Delete the existing CClipboardEntry's so that another attempt
						// at calling CopySelectionToClipboard can be made later after the
						// calling code has freed up some memory.
						delete_entries();
						return false;
					}
				}		
				
				p_entry = p_entry->GetNext();
			}
		}
	}	

	// Now one last ugly bit. Raising the selection may have moved some simple ground pieces
	// to above the ground, in which case they won't have pieces filling the gap, which looks odd.
	// So create these pieces if needed.
	CClipboardEntry *p_entry=mp_entries;
	while (p_entry)
	{
		if (!p_entry->CreateGapFillerPieces())
		{
			// Could not create the pieces, due to low memory.
			delete_entries();
			return false;
		}	
		p_entry=p_entry->mpNext;
	}


	Dbg_MsgAssert(mp_rails==NULL,("Expected mp_rails=NULL"));
	Mth::Vector corner_pos;
	Mth::Vector area_dims;
	corner_pos=Ed::CParkManager::Instance()->GridCoordinatesToWorld(area,&area_dims);
	mp_rails=Obj::GetRailEditor()->GenerateDuplicateRails(corner_pos[X], corner_pos[Z], 
														  corner_pos[X]+area_dims[X], corner_pos[Z]+area_dims[Z]);
		
	return true;
}

bool CClipboard::IsEmpty()
{
	if (mp_entries)
	{
		return false;
	}
	if (mp_rails)
	{
		return false;
	}	
	return true;
}
		
// Pastes the contents of the clipboard into the position that the cursor is at,
// using the cursor's rotation value.
bool CClipboard::Paste(CCursor *p_cursor)
{
	Spt::SingletonPtr<CParkEditor> p_editor;
	if (!p_editor->RoomToCopyOrPaste(m_max_x-m_min_x+1, m_max_z-m_min_z+1))
	{
		return false;
	}	

	// Get the map coords of the centre cell of the set of meta's
	uint8 centre_x=(m_min_x+m_max_x)/2;
	uint8 centre_z=(m_min_z+m_max_z)/2;
	
	// Bail out if any entry cannot be pasted
	CClipboardEntry *p_entry=mp_entries;
	while (p_entry)
	{
		if (!p_entry->CanPaste(centre_x,centre_z,p_cursor))
		{
			return false;
		}	
		p_entry=p_entry->mpNext;
	}	

	// Bail out if there are not enough free rail points to create the edited rails
	int num_rail_points_to_paste=0;
	Obj::CEditedRail *p_rail=mp_rails;
	while (p_rail)
	{
		num_rail_points_to_paste += p_rail->CountPoints();
		p_rail=p_rail->mpNext;
	}
	if (num_rail_points_to_paste > Obj::GetRailEditor()->GetNumFreePoints() )
	{
		return false;
	}	

	
	// OK to proceed, so paste the entries
	p_entry=mp_entries;
	while (p_entry)
	{
		p_entry->Paste(centre_x,p_cursor->GetClipboardY(),centre_z,p_cursor);
		p_entry=p_entry->mpNext;
	}	
	
	// and paste the rails
	Mth::Vector clipboard_pos=GetClipboardWorldPos();
	Mth::Vector cursor_pos=p_editor->GetCursorPos();
	
	p_rail=mp_rails;
	while (p_rail)
	{
		Obj::CEditedRail *p_new_rail=Obj::GetRailEditor()->NewRail();
		p_new_rail->CopyRail(p_rail);
		p_new_rail->RotateAndTranslate(clipboard_pos, cursor_pos, p_cursor->GetRotation()*90.0f);
		
		p_new_rail->UpdateRailGeometry();
		p_new_rail->UpdatePostGeometry();
		
		p_rail=p_rail->mpNext;
	}
		
	return true;
}


int CClipboard::GetFloorHeight(CCursor *p_cursor)
{
	// Get the map coords of the centre cell of the set of meta's
	uint8 centre_x=(m_min_x+m_max_x)/2;
	uint8 centre_z=(m_min_z+m_max_z)/2;

	int max_height=-1000;	
	CClipboardEntry *p_entry=mp_entries;
	while (p_entry)
	{
		int height=p_entry->GetFloorHeight(centre_x,centre_z,p_cursor);
		if (height > max_height)
		{
			max_height=height;
		}	
		p_entry=p_entry->mpNext;
	}	
	
	if (max_height==-1000)
	{
		return 0;
	}
	return max_height;
}

int CClipboard::FindMaxFloorHeight(CCursor *p_cursor)
{
	// Get the map coords of the centre cell of the set of meta's
	uint8 centre_x=(m_min_x+m_max_x)/2;
	uint8 centre_z=(m_min_z+m_max_z)/2;

	int max_floor_height=-1000;	
	CClipboardEntry *p_entry=mp_entries;
	while (p_entry)
	{
		int height=p_entry->GetFloorHeight(centre_x,centre_z,p_cursor);
		if (height > max_floor_height)
		{
			max_floor_height=height;
		}	
			
		p_entry=p_entry->mpNext;
	}	
	
	if (max_floor_height != -1000)
	{
		return max_floor_height;
	}
	return 0;	
}

void CClipboard::HighlightIntersectingMetas(CCursor *p_cursor, bool on)
{
	// Get the map coords of the centre cell of the set of meta's
	uint8 centre_x=(m_min_x+m_max_x)/2;
	uint8 centre_z=(m_min_z+m_max_z)/2;
	
	CClipboardEntry *p_entry=mp_entries;
	while (p_entry)
	{
		p_entry->HighlightIntersectingMetas(centre_x,centre_z,p_cursor,on);
		p_entry=p_entry->mpNext;
	}	
}

int CClipboard::GetWidth()
{
	return m_max_x-m_min_x+1;
}

int CClipboard::GetLength()
{
	return m_max_z-m_min_z+1;
}

void CClipboard::GetArea(CCursor *p_cursor, GridDims *p_area)
{
	// Get the map coords of the centre cell of the set of meta's
	uint8 centre_x=(m_min_x+m_max_x)/2;
	uint8 centre_z=(m_min_z+m_max_z)/2;
	
	int idx=m_min_x-centre_x;
	int idz=m_min_z-centre_z;
	int dx=0;
	int dz=0;
	
	switch ( p_cursor->GetRotation() )
	{
		case Mth::ROT_0:
			dx=idx;
			dz=idz;
			p_area->SetWHL(m_area.GetW(),1,m_area.GetL());
			break;
		case Mth::ROT_270:
			dx=-idz;
			dz=idx;
			p_area->SetWHL(m_area.GetL(),1,m_area.GetW());
			// Shift dx,dz so that they refer to the top-left point again
			dx=dx-m_area.GetL()+1;
			break;
		case Mth::ROT_180:
			dx=-idx;
			dz=-idz;
			p_area->SetWHL(m_area.GetW(),1,m_area.GetL());
			// Shift dx,dz so that they refer to the top-left point again
			dx=dx-m_area.GetW()+1;
			dz=dz-m_area.GetL()+1;
			break;
		case Mth::ROT_90:
			dx=idz;
			dz=-idx;
			p_area->SetWHL(m_area.GetL(),1,m_area.GetW());
			// Shift dx,dz so that they refer to the top-left point again
			dz=dz-m_area.GetW()+1;
			break;
		default:
			break;
	}		

	GridDims cursor_position=p_cursor->GetPosition();
	p_area->SetXYZ(cursor_position.GetX()+dx,0,cursor_position.GetZ()+dz);
}

Mth::Vector CClipboard::GetClipboardWorldPos()
{
	// Get the map coords of the centre cell of the set of meta's
	uint8 centre_x=(m_min_x+m_max_x)/2;
	uint8 centre_z=(m_min_z+m_max_z)/2;
	
	GridDims centre_cell;
	centre_cell.SetXYZ(centre_x,m_area.GetY(),centre_z);
	centre_cell.SetWHL(1,1,1);
	
	Mth::Vector corner_pos;
	Mth::Vector area_dims;
	corner_pos=Ed::CParkManager::Instance()->GridCoordinatesToWorld(centre_cell,&area_dims);
	// corner_pos is the world pos of the top left corner of the centre cell
	
	Mth::Vector clipboard_pos=corner_pos;
	clipboard_pos[X]+=area_dims[X]/2.0f;
	clipboard_pos[Z]+=area_dims[Z]/2.0f;
	// clipboard_pos is the world pos of the centre of the cell
	
	return clipboard_pos;
}

// This gets called when the CCursor is in paste mode, and needs to display the clipboard contents at
// the cursor position.
void CClipboard::ShowMetas(Mth::Vector pos, float rot, int clipboardY)
{
	// Get the map coords of the centre cell of the set of meta's
	uint8 centre_x=(m_min_x+m_max_x)/2;
	uint8 centre_z=(m_min_z+m_max_z)/2;
	
	CClipboardEntry *p_entry=mp_entries;
	while (p_entry)
	{
		p_entry->ShowMeta(centre_x,clipboardY,centre_z,pos,rot);
		p_entry=p_entry->mpNext;
	}	
	
	
	Mth::Vector clipboard_pos=GetClipboardWorldPos();
	
	Obj::CEditedRail *p_rail=mp_rails;
	while (p_rail)
	{
		p_rail->UpdateRailGeometry(clipboard_pos,pos,rot);
		p_rail->UpdatePostGeometry(clipboard_pos,pos,rot);
		p_rail=p_rail->mpNext;
	}	
}

void CClipboard::DestroyMetas()
{
	CClipboardEntry *p_entry=mp_entries;
	while (p_entry)
	{
		p_entry->DestroyMeta();
		p_entry=p_entry->mpNext;
	}	

	Obj::CEditedRail *p_rail=mp_rails;
	while (p_rail)
	{
		p_rail->DestroyPostGeometry();
		p_rail->DestroyRailGeometry();
		p_rail=p_rail->mpNext;
	}	
}

} // namespace Ed

