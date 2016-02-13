#ifndef __GFX_2D_SCREENELEMMAN_H__
#define __GFX_2D_SCREENELEMMAN_H__

#include <gel/objman.h>
#include <gel/event.h>
#include <gfx/2D/ScreenElement2.h>

#include <sys/sioman.h>

namespace Script
{
	class CScriptStructure;
	class CScript;
}

namespace Front
{

/*
	Manages all screen elements. References the root element of the screen element
	parent/child tree.
	
	Keeps track of which elements are in focus, and makes sure that pad events are
	passed to these elements.
*/
class CScreenElementManager : public Obj::CBaseManager, public Obj::CEventListener
{

	DeclareSingletonClass( CScreenElementManager );

public:
								CScreenElementManager();
								~CScreenElementManager();

	CScreenElementPtr 			CreateElement(uint32 type, uint32 id, Script::CStruct *pProps);
	enum 						ERecurse { DONT_RECURSE = 0, RECURSE = 1};
	enum 						EPreserveParent { DONT_PRESERVE_PARENT = 0, PRESERVE_PARENT = 1};
	void						DestroyElement(uint32 id, ERecurse recurse = RECURSE, EPreserveParent preserveParent = DONT_PRESERVE_PARENT, Script::CScript *pCallingScript = NULL);

	void						SetParent(const CScreenElementPtr &pParent, const CScreenElementPtr &pChild, CScreenElement::EPosRecalc recalculatePosition = CScreenElement::vRECALC_POS);
	enum EAssert
	{
		DONT_ASSERT = 0,
		ASSERT,
	};
	CScreenElementPtr 			GetElement(uint32 id, EAssert assert = DONT_ASSERT);
	CScreenElementPtr 			GetElement(Script::CStruct *pStructContainingId, char *pIdSubStructName, EAssert assert = DONT_ASSERT);
	CScreenElementPtr 			GetElement(Script::CStruct *pStructContainingId, uint32 IdSubStructName, EAssert assert);

	void						Update();
	void						SetPausedState(bool pause);
	
	void 						set_tree_lock_state(CScreenElement::ELockState state);
	void						pass_event_to_listener(Obj::CEvent *pEvent);	
	
	bool						IsComplexID(Script::CStruct *pStructContainingId, char *pIdSubStructName);
	uint32 						ResolveComplexID(Script::CStruct *pStructContainingId, uint32 IdSubStructName);
	uint32						ResolveComplexID(Script::CStruct *pStructContainingId, char *pIdSubStructName);
	
	/*
		Virtual functions from CBaseManager
	*/
	
	void						RegisterObject ( Obj::CObject& obj );
	void						UnregisterObject ( Obj::CObject& obj );	
	void						KillObject ( Obj::CObject& obj );
	Lst::Head< Obj::CObject > &	GetRefObjectList();

	void						SetRootScreenElement( uint32 id );

private:

	struct FocusNode // in the focus tree
	{
		//FocusNode *				mpFirstChild;
		//FocusNode *				mpSibling;
		CScreenElementPtr 		mpElement;
		uint32					mId;
		FocusNode *				mpNextNode;
		bool					mProcessed;
		bool					mTempOutOfFocus;
	};
	
	void						destroy_element_recursive(EPreserveParent preserve_parent, const CScreenElementPtr &pElement, Script::CScript *pCallingScript);
	CScreenElementPtr 			get_element_by_local_id(const CScreenElementPtr &pParent, uint32 desiredLocalID);

	void						mark_element_in_focus(const CScreenElementPtr &pElement, int controller);
	void						mark_element_out_of_focus(const CScreenElementPtr &pElement, bool onlyChildren = false, bool temporaryOnly = false);
	void						remark_temporarily_out_of_focus_elements(const CScreenElementPtr &pElement);
	void 						test_focus_node(FocusNode *pNode);

	bool						is_pad_event(uint32 eventType);
	
	CScreenElementPtr			mp_root_element;

	// a temporary pointer used by ResolveComplexID() for recursion
	CScreenElementPtr 			mp_resolve_temp;
	
	enum
	{
		NUM_FOCUS_NODES	=		48,					// basically, a pool of these
		NUM_FOCUS_LISTS =		SIO::vMAX_DEVICES,	// one for each controller
	};
	
	FocusNode					m_focus_node_pool[NUM_FOCUS_NODES];	
	FocusNode *					mp_focus_list[NUM_FOCUS_LISTS];
	bool						m_focus_list_changed[NUM_FOCUS_LISTS];

	enum
	{
		MAX_PAD_EVENT_TYPES		= 48,
	};
	uint32						m_pad_event_type_tab[MAX_PAD_EVENT_TYPES];
	int							m_num_pad_event_types;
};



bool ScriptCreateScreenElement(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptDestroyScreenElement(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptRunScriptOnScreenElement(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptSetScreenElementProps(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptDoScreenElementMorph(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptSetScreenElementLock(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptScreenElementSystemInit(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptGetScreenElementDims(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptTextElementConcatenate(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptTextElementBackspace(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptGetTextElementString(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptGetTextElementLength(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptGetScreenElementPosition(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptMenuSelectedIndexIs(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptScreenElementExists(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptGetScreenElementProps( Script::CScriptStructure *pParams, Script::CScript *pScript );
bool ScriptSetRootScreenElement( Script::CStruct* pParams, Script::CScript* pScript );
}

#endif
