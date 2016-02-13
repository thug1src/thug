#ifndef	__SK_SCRIPTING_NODEARRAY_H
#define	__SK_SCRIPTING_NODEARRAY_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

namespace Mth
{
class Vector;
class Matrix;
}

namespace Script
{
class CStruct;
class CArray;
}	

namespace SkateScript
{
using namespace Script;

void InitNodeNameHashTable();
void ClearNodeNameHashTable();
void CreateNodeNameHashTable();
int FindNamedNode(uint32 checksum, bool assert=true);
int FindNamedNode(const char *p_name);
bool NodeExists(uint32 checksum);

void DeletePrefixInfo();
void GeneratePrefixInfo();
const uint16 *GetPrefixedNodes(const char *p_prefix, uint16 *p_numMatches);
const uint16 *GetPrefixedNodes(uint32 checksum, uint16 *p_numMatches);

int	GetNearestNodeByPrefix(uint32 prefix, const Mth::Vector &pos);
int	GetNearestNodeByPrefix(const char * p_prefix, const Mth::Vector &pos);


CStruct *GetNode(int nodeIndex);
uint32	GetNodeNameChecksum(int nodeIndex);
uint32 GetNumLinks(CStruct *p_node);
uint32 GetNumLinks(int nodeIndex);
int GetLink(CStruct *p_node, int linkNumber);
int GetLink(int nodeIndex, int linkNumber);
bool IsLinkedTo(int node1, int node2);

void GetPosition(CStruct *p_node, Mth::Vector *p_vector);
void GetPosition(int nodeIndex, Mth::Vector *p_vector);
void GetOrientation(CStruct *p_node, Mth::Matrix *p_matrix);
void GetOrientation(int nodeIndex, Mth::Matrix *p_matrix);
void GetAngles(CStruct *p_node, Mth::Vector *p_vector);
void GetAngles(int nodeIndex, Mth::Vector *p_vector);

CArray *GetIgnoredLightArray(CStruct *p_node);
CArray *GetIgnoredLightArray(int nodeIndex);

void CreateNodeArray(int size, bool hackUseFrontEndHeapYouFucker);
void ScanNodeScripts(uint32 componentName, uint32 functionName, void (*p_callback)(CStruct *, const uint8 *));

} // namespace SkateScript

#endif // #ifndef	__SK_SCRIPTING_NODEARRAY_H

