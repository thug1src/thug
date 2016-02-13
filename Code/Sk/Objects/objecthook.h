//////////////////////////////////////////
// ObjectHook.h

#ifndef __OBJECTS_OBJECTHOOK_H
#define __OBJECTS_OBJECTHOOK_H

namespace Script
{
	class	CArray;
	class	CStruct;
}



namespace Obj
{

class	CObjectHookManager;

enum EObjectHookNodeFlag
{
	GRAB_OVERRIDE = 0x01,
};

class CObjectHookNode : public Spt::Class
{
	friend class  CObjectHookManager;

public:						  
	const sint16			GetNode() const 	  		{return m_node;}
	void					SetActive(bool active)		{m_active = active;}
	const bool				IsActive() const				{return m_active;}

	void					SetFlag(EObjectHookNodeFlag flag) {m_flags |= flag;}
	void					ClearFlag(EObjectHookNodeFlag flag) {m_flags &= ~flag;}
	bool					GetFlag(EObjectHookNodeFlag flag) const {return m_flags & flag;}

	CObjectHookNode *		GetNext(){return m_pNext;}

protected:

	Mth::Vector m_pos;			// position of the node
	CObjectHookNode *m_pNext;			// Pointer to next node in master list	 	(or NULL if none)
	sint16 m_node;					// Number of the node in the trigger array
	char m_active;				// ObjectHook segment starting at this node: active or not?
	char m_flags;				// flags for ObjectHook segment

// the position getting functions are private
// so you can only access them through the ObjectHook manager
// (as you need to know if it's transformed or not)
private:
	const Mth::Vector&	GetPos() const;	 
						  
};



class CObjectHookManager : public Spt::Class
{

public:
					CObjectHookManager();
				   ~CObjectHookManager();	

	void 			Cleanup();
								 
	void			AddObjectHooksFromNodeArray(Script::CArray * p_node_array);
	void 			AddObjectHookNode(int node_number, Script::CStruct *p_node_struct);
	void 			AddedAll();
	void			UpdateTransform(Mth::Matrix& transform);
	
	void 			SetActive( int node, int active, bool wholeObjectHook );
	bool 			IsActive( int node );
	bool			IsMoving(){return m_is_transformed;}
	
	void			DebugRender(Mth::Matrix *p_transform = NULL);
	void			RemoveOverlapping();
	
	CObjectHookNode*		GetFirstNode(){return mp_first_node;}
	
	Script::CArray *	GetNodeArray(){return mp_node_array;}
	void				SetNodeArray(Script::CArray *p_nodeArray){mp_node_array = p_nodeArray;}

// accessor functions for the ObjectHook node
// we only access their positions via the ObjectHook manager
// as only the ObjectHook manager knows the transform of the object to
// which they are attached
// These nodes are "instances", in that the same node might be used in
// more than one place
// so these functions only make sense in conjunction with the ObjectHook_manager
	Mth::Vector	GetPos(const CObjectHookNode *p_node) 	const;	 

private:
	CObjectHookNode*  			mp_first_node;	
	bool						m_is_transformed;
	Mth::Matrix					m_transform;
	Script::CArray *			mp_node_array;

};


} 

			
#endif			// #ifndef __OBJECTS_OBJECTHOOK_H
			


