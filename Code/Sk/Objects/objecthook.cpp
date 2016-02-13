///////////////////////////////////////////////////////
// ObjectHook.cpp

#include <core/defines.h>
#include <core/singleton.h>
#include <core/math.h>
#include <sk/objects/ObjectHook.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/script.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <sk/scripting/nodearray.h>
#include <gfx/debuggfx.h>
#include <sk/modules/skate/skate.h>
#include <gel/modman.h>

#include <core/math/geometry.h>
#include <engine/feeler.h>

#ifdef	__PLAT_NGPS__	
#include <gfx/ngps/nx/line.h>
#endif




namespace Obj
{


CObjectHookManager::CObjectHookManager()
{
	mp_first_node = NULL;
	m_is_transformed = false;
	mp_node_array = NULL;
	
}

CObjectHookManager::~CObjectHookManager()
{	 
	Cleanup();  
}

void CObjectHookManager::Cleanup()
{
	while (mp_first_node)
	{
		CObjectHookNode *pNext = mp_first_node->m_pNext;
		delete mp_first_node;
		mp_first_node = pNext;			
	}    
	m_is_transformed = false;	
	// Unhook from node array
	mp_node_array = NULL;
}


void CObjectHookManager::AddObjectHookNode(int node_number, Script::CStruct *p_node_struct)
{

	// create a new ObjectHook node										   
	CObjectHookNode *pObjectHookNode = new CObjectHookNode();
	// Link into the head of the ObjectHook manager	
	// Note, this must be done before we attempt to link up with other nodes								   
	pObjectHookNode->m_pNext = mp_first_node; 							  
	mp_first_node = pObjectHookNode;

	pObjectHookNode->m_node = node_number;			// The node_number is use primarily for calculating links
	pObjectHookNode->m_active = p_node_struct->ContainsFlag( 0x7c2552b9 /*"CreatedAtStart"*/ );	// created at start or not?
	pObjectHookNode->m_flags = 0;						// all flags off by default

	SkateScript::GetPosition(p_node_struct,&pObjectHookNode->m_pos);

}

void CObjectHookManager::UpdateTransform(Mth::Matrix& transform)
{

	m_is_transformed = true;
	m_transform = transform;
}



void	CObjectHookManager::DebugRender(Mth::Matrix *p_transform)
{
	#ifdef	__PLAT_NGPS__

	Mth::Vector up(0.0f,1.0f,0.0f);	

	uint32	rgb = 0x0000ff;
	
	uint32 current = 12345678;

	
	
	CObjectHookNode *pObjectHookNode;
	pObjectHookNode = mp_first_node;
	int count = 0;
	// then draw the node positions as litle red lines	
	pObjectHookNode = mp_first_node;
	while (pObjectHookNode)
	{
		count++;
		if (pObjectHookNode->m_active)
		{
			rgb = 0xff0000;
		}
		else
		{
			rgb = 0x00ff00;
		}
	
		// check for context changes
		if (current != rgb)
		{
			if ( current != 12345678)
			{
				NxPs2::EndLines3D();
			}
			
			current = rgb;
			NxPs2::BeginLines3D(0x80000000 + (0x00ffffff & rgb));
		}
	
		Mth::Vector v0, v1;
		v0 = pObjectHookNode->m_pos;
		v1 = v0 + up * 24.0f;
		v0[W] = 1.0f;		// Make homgeneous
		v1[W] = 1.0f;

		if (p_transform)
		{
			v0 = p_transform->Transform(v0);
			v1 = p_transform->Transform(v1);
		}

		NxPs2::DrawLine3D(v0[X],v0[Y],v0[Z],v1[X],v1[Y],v1[Z]);
		
		pObjectHookNode = pObjectHookNode->m_pNext;
		
	}

	
	
	// only if we actually drew some
	if ( current != 12345678)
	{
		NxPs2::EndLines3D();
	}


	#endif

}




void	CObjectHookManager::AddObjectHooksFromNodeArray(Script::CArray * p_nodearray)
{
	Dbg_MsgAssert(!mp_node_array,("Setting two node arrays in ObjectHook manager"));
	mp_node_array = p_nodearray;
	
	
	uint32 i;	 
	for (i=0; i<p_nodearray->GetSize(); ++i)
	{
		Script::CStruct *p_node_struct=p_nodearray->GetStructure(i);
		Dbg_MsgAssert(p_node_struct,("Error getting node from node array for ObjectHook generation"));

		uint32 class_checksum = 0;		
		p_node_struct->GetChecksum( 0x12b4e660 /*"Class"*/, &class_checksum );
		if (class_checksum == 0xd18719d5)		  // checksum 'ObjectHook'
		{
			AddObjectHookNode( i, p_node_struct);
		}
	}
}


const Mth::Vector&	CObjectHookNode::GetPos() const
{
	return m_pos;
}


Mth::Vector	CObjectHookManager::GetPos(const CObjectHookNode *p_node) 	const
{
	if (!m_is_transformed)
	{
		return p_node->GetPos();
	}
	else
	{
		Mth::Vector v = m_transform.Transform(p_node->GetPos());
		return v;
	}
}


}
			   

