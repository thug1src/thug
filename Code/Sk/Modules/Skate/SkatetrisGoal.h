// skatetris subclass

#ifndef __SK_MODULES_SKATE_SKATETRISGOAL_H__
#define __SK_MODULES_SKATE_SKATETRISGOAL_H__

#ifndef __CORE_DEFINES_H
    #include <core/defines.h>
#endif

#include <sk/modules/skate/GoalManager.h>

namespace Game
{

const int vMAXTETRISTRICKS		= 30;

struct tetrisTrick 
{
	uint32 		trickNameChecksum;
	uint32		altTrickNameChecksum;
    uint32 		keyCombo;
    uint32 		screenElementId;
    bool 		invalid;
	bool		require_perfect;
	int 		group_id;
	int 		spin_mult;
	int 		num_taps;
};

class CSkatetrisGoal : public CGoal
{

public:
						CSkatetrisGoal( Script::CStruct* pParams );
	virtual				~CSkatetrisGoal();

	bool				Activate();
	bool				Deactivate( bool force = false, bool affect_tree = true );
	bool				Win();
	void				Expire();
	
	bool				Update();

	void                StartTetrisGoal();
	void                EndTetrisGoal();
	void                CheckTetrisTricks();
	void				CheckTetrisCombos();
	void                AddTetrisTrick( int num_to_add = 1 );
	void				UpdateFadedTricks();

	void				RemoveTrick( int index );

	bool				AllTricksCleared();

	void				ClearAllTricks();

	bool				IsSingleComboSkatetris();
	bool				IsComboSkatetris();
	bool				IsTricktris();
protected:

    tetrisTrick         m_tetrisTricks[vMAXTETRISTRICKS];
    int                 m_numTetrisTricks;
    int					m_tetrisTime;
	bool				m_validGroups[vMAXTETRISTRICKS];
};

}

#endif

