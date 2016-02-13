/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		<skate3>											**
**																			**
**	Module:			<object>			 								**
**																			**
**	File name:		restart.cpp											**
**																			**
**	Created by:		00/00/00	-	initials								**
**																			**
**	Description:	<description>		 									**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <gel/scripting/script.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>
#include <sk/scripting/nodearray.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Obj
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/

/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

// Return tru is this node is a restart node
bool IsRestart(Script::CScriptStructure *pNode)
{
	uint32	ClassChecksum;
	pNode->GetChecksum("Class",&ClassChecksum);
	if (ClassChecksum == 0x1806ddf8)					// checksum of "Restart"
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool IsRestart(int node)
{
	// get the node array	
	Script::CArray *pNodeArray=Script::GetArray(CRCD(0xc472ecc5,"NodeArray"));
	Script::CScriptStructure *pNode=pNodeArray->GetStructure(node);
	return	IsRestart(pNode);
}

int FindQualifyingNodes( uint32 type, uint32 *node_buffer, int first = -1 )
{
	int num_qualifiers;
	int i;

	num_qualifiers = 0;
	i = 0;
	
	// get the node array	
	Script::CArray *pNodeArray=Script::GetArray(CRCD(0xc472ecc5,"NodeArray"));

	// if there is no node array, then there are no restart points
	if (!pNodeArray)
	{
		return 0;
	}
						 
	if( first >= 0 )
	{
		i = first + 1;
	}
	// scan through it from the start point
	while( i < (int)pNodeArray->GetSize() )
	{
		Script::CScriptStructure *pNode=pNodeArray->GetStructure(i);
		if (IsRestart(pNode))					// checksum of "Restart"
		{   
			Script::CArray *pArray;
			int j;

			// The caller wants all restart points
			if (type == 0)
			{
				node_buffer[num_qualifiers++] = i;
				i++;
				continue;
			}

			// Loop through supported game types, gathering matches
			if (pNode->GetArray("restart_types", &pArray))
			{
				for (j = 0; j < (int)pArray->GetSize(); j++)
				{
					uint32 game_type;
						
					game_type = pArray->GetNameChecksum( j );
					if( game_type == type )
					{
						node_buffer[num_qualifiers++] = i;
					}
				}
			}
		}		
		i++;	
	}	

	return num_qualifiers;
}


// given the checksum of a type of restart
// then get the node index  
// if "first" is specified then resturn the first on after this								  
// if "type"  is zero, then it will be ignored, and will return restarts of any type.
// returns -1 if no restarts matching "type" are found.
int GetRestartNode(uint32 type, int index, int first = -1)
{
	// decide where to start, based on the "first" parameter
	int num_qualifiers;
	uint32 qualifying_nodes[256];
	
	num_qualifiers = FindQualifyingNodes( type, qualifying_nodes, first );
	if( num_qualifiers == 0 )
	{
		return -1;
	}
	
	#ifdef	__NOPT_ASSERT__
	if (num_qualifiers > 1 && (type == CRCD(0xbae0e4da,"multiplayer") || type == CRCD(0x9d65d0e7,"horse")))
	{
		// Scan through the nodes we've found, and check that there are not any
		// that are too close to each other (< 6 feet)
		bool ok = true;
		for (int i=0;i<num_qualifiers-1;i++)
		{
			for (int j=i+1;j<num_qualifiers;j++)
			{
				Dbg_MsgAssert(qualifying_nodes[i] != qualifying_nodes[j],("Restart node %d qualifies more than once!\n",qualifying_nodes[i]));
				Mth::Vector i_pos, j_pos;
				SkateScript::GetPosition( qualifying_nodes[i], &i_pos );
				SkateScript::GetPosition( qualifying_nodes[j], &j_pos );
				float dist = (i_pos-j_pos).Length();
				if (dist < 12.0f*6.0f)
				{
					if (ok) printf ("\n\n\n");
					ok = false;
					uint32	i_name_checksum;
					SkateScript::GetNode(qualifying_nodes[i])->GetChecksum("Name",&i_name_checksum);
					uint32	j_name_checksum;
					SkateScript::GetNode(qualifying_nodes[j])->GetChecksum("Name",&j_name_checksum);
					printf ("%2.2f"" %s to %s \n",
					 dist,
					 Script::FindChecksumName(i_name_checksum),
					 Script::FindChecksumName(j_name_checksum)
					 );
				}
			}
		}
		if (!ok) printf ("\n\n\n");
		Dbg_MsgAssert(ok, ("Restart nodes of type %s too close, see above for list\n",Script::FindChecksumName(type)));
	}
	#endif	
	
	//Ryan("index is %d, num_qualifiers %d\n", index, num_qualifiers);
	
	// If we've found matches, return the 
	if( num_qualifiers > 0 )
	{
		if( index >= num_qualifiers )
		{
			return qualifying_nodes[index % num_qualifiers];
		}
		else
		{
			return qualifying_nodes[index];
		}
	}
	return -1;
}
				
				
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Obj




