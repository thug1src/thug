/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			SSEC													**
**																			**
**	File name:		SuperSector.h											**
**																			**
**	Created: 		01/12/2001	-	spg										**
**																			**
*****************************************************************************/

#ifndef	__ENGINE_SUPERSECTOR_H
#define	__ENGINE_SUPERSECTOR_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#include <core/math.h>
#include <core/math/geometry.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/////////////////////////////////////////////////////////////////////////
// Forward declarations
namespace Nx
{
class	CSector;
class	CCollObj;
class	CCollStatic;
}


namespace SSec
{

// Forward declaration
class Manager;

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class Sector
{
public:
	Sector( void );
	~Sector( void );

private:
	// Remove pointer out of sector array and shift the rest.  Doesn't update m_NumCollSectors!
	bool				remove_collision(Nx::CCollStatic *p_coll);
	bool				replace_collision(Nx::CCollStatic *p_coll, Nx::CCollStatic *p_replace_coll);
	bool				has_collision(Nx::CCollStatic *p_coll);

	Mth::CBBox			m_Bbox;
	int					m_NumSectors;
	int					m_NumCollSectors;
	Nx::CSector**		m_SectorList;
	Nx::CCollStatic**	m_CollSectorList;

	friend Manager;
};


class Manager
{
public:
	enum
	{
		NUM_PARTITIONS_X = 20,
		NUM_PARTITIONS_Z = 20,
	};

	Nx::CSector**		GetIntersectingWorldSectors( Mth::Line &line );
	Nx::CCollStatic**	GetIntersectingCollSectors( Mth::Line &line );
	Nx::CCollStatic**	GetIntersectingCollSectors( Mth::CBBox &bbox );
	void				GenerateSuperSectors( const Mth::CBBox& world_bbox );
	void				AddCollisionToSuperSectors( Nx::CCollStatic *coll, int num_coll_sectors );
	void				UpdateCollisionSuperSectors(Lst::Head<Nx::CCollStatic> &add_list,
													Lst::Head<Nx::CCollStatic> &remove_list,
													Lst::Head<Nx::CCollStatic> &update_list);
	void				ClearCollisionSuperSectors();

	Mth::CBBox *		GetWorldBBox( void ) { return &m_world_bbox; }

private:
	int					add_to_super_sector(Nx::CCollStatic *p_coll, SSec::Sector *p_super_sector, Lst::Head< Nx::CCollStatic > & add_list, int num_changes);
	int					remove_from_super_sector(Nx::CCollStatic *p_coll, SSec::Sector *p_super_sector, Lst::Head< Nx::CCollStatic > & add_list, int num_changes);

	Mth::CBBox			m_world_bbox;
	float				m_sector_width;
	float 				m_sector_depth;
	int 				m_num_sectors_x;
	int 				m_num_sectors_z;
	Sector				m_super_sector_list[NUM_PARTITIONS_X][NUM_PARTITIONS_Z];
};


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

uint8		GetCurrentSectorOperationId( void );
uint8		GetNewSectorOperationId( void );


/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace SSec

#endif	// __ENGINE_SUPERSECTOR_H