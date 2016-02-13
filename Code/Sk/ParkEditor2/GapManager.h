#ifndef __SK_PARKEDITOR2_GAPMANAGER_H
#define __SK_PARKEDITOR2_GAPMANAGER_H

#include <sk/ParkEditor2/ParkGen.h>

namespace Script
{
	class CStruct;
	class CScript;
}


namespace Ed
{

class CGapManager;
class GapInfo;
class CParkManager;

class  CGapManager
{
	
	
public:

	struct GapDescriptor
	{
		GridDims				loc[2];
		int						rot[2];
		int 					leftExtent[2];
		int						rightExtent[2];
		char 					text[32];
		int						score;
		int						numCompleteHalves; // will be 0, 1, or 2
		int						tabIndex; // set to -1 if not in table
		
		// An 'or' of the CANCEL_ values as defined in skutils.q, eg CANCEL_GROUND
		uint32					mCancelFlags;
	};
	
	// actually max gap HALVES, divide by two to get gaps
	static const int			vMAX_GAPS = 32;

								CGapManager(CParkManager *pManager);
								~CGapManager();

	int 						GetFreeGapIndex();
	void						StartGap(CClonedPiece *pGapPiece, CConcreteMetaPiece *pRegularMeta, int tab_index);
	void						EndGap(CClonedPiece *pGapPiece, CConcreteMetaPiece *pRegularMeta, int tab_index);
	void						SetGapInfo(int tab_index, GapDescriptor &descriptor);
	bool						IsGapAttached(const CConcreteMetaPiece *pRegularMeta, int *pTabIndex);
	void						RemoveGap(int tab_index, bool unregisterGapFromGoalEditor=true);
	void						RemoveAllGaps();
	void						RegisterGapsWithSkaterCareer();

	uint32						GetGapTriggerScript(CClonedPiece *pGapPiece);
	GapDescriptor *				GetGapDescriptor(int tab_index, int *pHalfNum);
	void						MakeGapWireframe(int tab_index);
	
	void						LaunchGap(int tab_index, Script::CScript *pScript);

	static CGapManager *		sInstance();
	void						SetInstance() {sp_instance=this;}

	static CGapManager *		sInstanceNoAssert() {return sp_instance;}

	CGapManager& operator=( const CGapManager& rhs );
	
private:

	uint32						m_currentId;

	GapInfo *					mp_gapInfoTab[vMAX_GAPS];
	bool						m_startOfGap[vMAX_GAPS];

	CParkManager				*mp_manager;
	
	static CGapManager *		sp_instance;
};




class  GapInfo  : public Spt::Class
{
	
	friend class CGapManager;

private:

	GapInfo();

	uint32						m_id[2];
	CClonedPiece *				mp_gapPiece[2];
	CConcreteMetaPiece *		mp_regularMeta[2];
	char						mp_text[128];

	CGapManager::GapDescriptor	m_descriptor;
};




}

#endif

