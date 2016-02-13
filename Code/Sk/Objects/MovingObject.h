//****************************************************************************
//* MODULE:         Sk/Objects
//* FILENAME:       MovingObject.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/25/2000
//****************************************************************************

#ifndef __OBJECTS_MOVINGOBJECT_H
#define __OBJECTS_MOVINGOBJECT_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/support.h>

#include <core/math.h>
#include <core/task.h>

#include <gel/object/compositeobject.h>

/*****************************************************************************
**							Forward Declarations							**
*****************************************************************************/

namespace Gfx
{
	class 	CSkeleton;
	class 	CModelAppearance;
};
											  
namespace Nx
{
    class 	CModel;
	class 	CCollObj;
};

namespace Obj
{
    class 	CFollowOb;
	class	CRailManager;
	class	CObjectHookManager;
    
/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**							   Class Definitions							**
*****************************************************************************/

struct SiteBox
{
	Mth::Matrix     matrix;
	float           height;
	float           dist;
	float           width;
	float           angle;
	SBBox           bbox;
	bool            initialized;
	bool            maxDistInitialized;
	float           maxDistSquared;

#ifdef __NOPT_ASSERT__	
	bool            debug;
#endif
};

class CMovingObject : public CCompositeObject 
{
public :
		CMovingObject();
		virtual ~CMovingObject( void );

public:
		void					MovingObjectCreateComponents();
		void 					MovingObjectInit( Script::CStruct* pNodeData, CGeneralManager* p_obj_man );

		virtual bool			CallMemberFunction( uint32 Checksum, Script::CStruct *pParams, Script::CScript *pScript );

		Gfx::CSkeleton*			GetSkeleton( void );
		Nx::CModel*				GetModel( void );
				
protected:
		void					FillInSiteBox(SiteBox *p_siteBox, Script::CStruct *pParams);
		bool					ObjInSiteBox( CMovingObject *pObj, SiteBox *pBox );

public:
//		bool					IsFlipped( void );
		void					GetHoverOrgPos( Mth::Vector *p_orgPos );
		
protected:
		bool					ObjectWithinRange( Script::CStruct *pParams, Script::CScript *pScript, SiteBox *pBox = NULL );
		bool 					ObjectWithinRect( Script::CStruct *pParams, Script::CScript *pScript );
		bool					ObjectFromNodeWithinRange( int nodeIndex, int radiusSqr, SiteBox *pBox );
		void					InitializeSiteBox( SiteBox *pBox );
		float					GetSiteBoxMaxRadiusSquared( SiteBox *pBox );
		bool					ObjTypeInRange( Script::CStruct *pParams, Script::CScript *pScript, float radiusSqr, SiteBox *pBox, uint32 typeChecksum );
		bool					SkaterInRange( SiteBox *pBox, float radiusSqr );
		
protected:
		void					DrawBoundingBox( SBBox *pBox, Mth::Vector *pOffset = NULL, int numFrames = 1, Mth::Vector *pRot = NULL );
		
		void					ToggleFlipState( void );

protected:
		// float			   		m_time;					// time since last frame
  
		// the rest of the flags stay here...
//	    int						m_general_flags;
		
		// will add rotation vector (or rotation class?) to this ASAP...

private:		
		bool					LookAtObject_Init( Script::CStruct *pParams, Script::CScript *pScript );
		void					FollowLeader_Init( Script::CStruct *pParams );

		// GJ:  MOVED THESE FROM POSOBJECT.H
public:
		// Currently used by the PauseSkaters script command, which is used to pause all the skaters whilst
		// a camera animation is running.
		virtual void 			Pause() {mPaused=true; CCompositeObject::Pause(true);}
		virtual void 			UnPause() {mPaused=false; CCompositeObject::Pause(false);}
		bool		 			IsPaused() {return mPaused;}

		SBBox					m_bbox;

	protected:
		CCompositeObject*		GetClosestObjectOfType( int type );

	public :
		// Whether the object is paused.
		bool					mPaused;
};
							  
/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

} // namespace Obj

#endif	// __OBJECTS_MOVINGOBJECT_H
