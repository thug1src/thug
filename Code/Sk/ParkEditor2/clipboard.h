#ifndef __SK_PARKEDITOR2_CLIPBOARD_H
#define __SK_PARKEDITOR2_CLIPBOARD_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef __SYS_MEM_POOLABLE_H
#include <sys/mem/poolable.h>
#endif

#ifndef __SK_PARKEDITOR2_PARKGEN_H
#include <sk/parkeditor2/parkgen.h>
#endif

namespace Obj
{
	class CEditedRail;
}
	
namespace Ed
{

class CAbstractMetaPiece;

enum
{
	MAX_CLIPBOARD_METAS=1000,
	MAX_CLIPBOARDS=6,
	CLIPBOARD_MAX_W=10,
	CLIPBOARD_MAX_L=10,
};

class CClipboardEntry : public Mem::CPoolable<CClipboardEntry>
{
public:
	CClipboardEntry();
	~CClipboardEntry();

	void CalculateMapCoords(GridDims *p_mapCoords, uint8 centre_x, uint8 centre_z, CCursor *p_cursor);
	bool CanPaste(uint8 centre_x, uint8 centre_z, CCursor *p_cursor);
	void Paste(uint8 centre_x, int raiseAmount, uint8 centre_z, CCursor *p_cursor);
	int	 GetFloorHeight(uint8 centre_x, uint8 centre_z, CCursor *p_cursor);
	void HighlightIntersectingMetas(uint8 centre_x, uint8 centre_z, CCursor *p_cursor, bool on);
	void ShowMeta(uint8 centre_x, int raiseAmount, uint8 centre_z, Mth::Vector pos, float rot);
	void DestroyMeta();
	bool CreateGapFillerPieces();
	
	CAbstractMetaPiece *mpMeta;
	uint8 mX;
	sint8 mY;
	uint8 mZ;
	Mth::ERot90 m_rot;
	uint8 mWidth;
	uint8 mLength;
	
	// Used when displaying the cursor
	CConcreteMetaPiece *mpConcreteMeta;
	
	CClipboardEntry *mpNext;
};

class CClipboard : public Mem::CPoolable<CClipboard>
{
	GridDims m_area;
	
	uint8 m_min_x;
	uint8 m_min_z;
	uint8 m_max_x;
	uint8 m_max_z;
	void update_extents(uint8 x, uint8 z);
	
	CClipboardEntry *mp_entries;
	void delete_entries();
	
	Obj::CEditedRail *mp_rails;
	
public:
	CClipboard();
	~CClipboard();

	bool AddMeta(CConcreteMetaPiece *p_meta, int raiseAmount);
	bool AddHeight(int x, int y, int z);
	bool CopySelectionToClipboard(GridDims area);
	bool IsEmpty();
	bool Paste(CCursor *p_cursor);
	int	 GetFloorHeight(CCursor *p_cursor);
	int	 FindMaxFloorHeight(CCursor *p_cursor);
	void HighlightIntersectingMetas(CCursor *p_cursor, bool on);
	void ShowMetas(Mth::Vector pos, float rot, int clipboardY);
	void DestroyMetas();
	void GetArea(CCursor *p_cursor, GridDims *p_area);
	Mth::Vector GetClipboardWorldPos();

	int GetWidth();
	int GetLength();
	
	CClipboard *mpNext;	
};

}

#endif


