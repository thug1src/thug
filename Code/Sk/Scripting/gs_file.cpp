///////////////////////////////////////////////////////////////////////////////////////
//
// gs_file.cpp		KSH 7 Nov 2001
//
// Game specific functions for loading and unloading qb files.
// These call the LoadQB and UnloadQB functions in the Skate namespace, but then do
// any additional skate-specific processing required.
// For example, if the newly loaded qb contained a NodeArray, node name prefix info
// needs to be regenerated, etc.
// Also, skater exceptions may need to be reloaded.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <sk/scripting/gs_file.h>
#include <gel/scripting/file.h>
#include <gel/scripting/parse.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/string.h>
#include <gel/scripting/array.h>
#include <gel/scripting/struct.h>
#include <sk/scripting/nodearray.h>
#include <core/crc.h> // For Crc::GenerateCRCFromString
#include <sk/components/goaleditorcomponent.h>
#include <sys/file/pip.h>
#include <sk/objects/pathman.h>
#include <core/math.h>
#ifdef __PLAT_NGC__
#include <sys/config/config.h>
#endif		// __PLAT_NGC__

#include "string.h"

#ifdef __PLAT_NGC__
#include <dolphin.h>
#include "sys/ngc/p_display.h"
#endif		// __PLAT_NGC__

namespace SkateScript
{
using namespace Script;

// The size of the temporary buffer used when parsing the qdir.txt file. Used in LoadAllStartupQBFiles()
#define MAX_QB_FILENAME_CHARS_WITH_PATH 100

#define QDIR_FILE "scripts\\qdir.txt"
#define MAIN_QB_FILEPATH "scripts\\"

void LoadAllStartupQBFiles()
{
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
#ifdef __PLAT_NGC__
	switch ( Config::GetLanguage() )
	{
		case Config::LANGUAGE_FRENCH:
			Pip::LoadPre("qb.prf");
			break;
		case Config::LANGUAGE_GERMAN:
			Pip::LoadPre("qb.prd");
			break;
		default:
			Pip::LoadPre("qb.pre");
			break;
	}
#else
	Pip::LoadPre("qb.pre");
#endif		// __PLAT_NGC__
	Mem::Manager::sHandle().PopContext();

	// Load the qdir.txt file
	const char *p_qdir=(const char *)Pip::Load(QDIR_FILE);
	uint32 file_size=Pip::GetFileSize(QDIR_FILE);
	
	UsePermanentStringHeap();
	
    // Load each of the files listed.
	#define MAX_FILENAME_CHARS 200
	char p_file_name[MAX_FILENAME_CHARS+1];
	
	const char *p_scan=p_qdir;
	while (p_scan < p_qdir+file_size)
	{
		int c=0;
		while (p_scan < p_qdir+file_size && *p_scan!=0x0d && *p_scan!=0x0a)
		{
			Dbg_MsgAssert(c<MAX_FILENAME_CHARS,("File name too long"));
			p_file_name[c++]=*p_scan++;
		}
		p_file_name[c]=0;
		// If the above loop broke out because *p_scan was 0x0d or 0x0a then
		// this will skip over it. If p_scan was >= p_qdir+file_size then it
		// still will be after incrementing p_scan so that's OK too.
		++p_scan;
		
		// Skip any empty strings, which will happen when encountering 0x0d,0x0a pairs.
		if (*p_file_name)
		{
			// Note: p_file_name will contain the complete path, eg c:\skate5\data\scripts\blaa.qb

			// Strip off any preceding data path.			
			char *p_data_backslash=strstr(p_file_name,"data\\");
			if (p_data_backslash)
			{
				strcpy( p_file_name, p_data_backslash+5); // Safe cos it will copy backwards
			}
				
			// Make sure this script isn't already loaded.
			SkateScript::UnloadQB( Crc::GenerateCRCFromString(p_file_name) );

			SkateScript::LoadQB(p_file_name,
								// We do want assertions if duplicate symbols when loading all 
								// the qb's on startup. (Just not when reloading a qb)
								ASSERT_IF_DUPLICATE_SYMBOLS);
		}	
#ifdef __PLAT_NGC__
		NsDisplay::doReset();
#endif		// __PLAT_NGC__

	}	

	UseRegularStringHeap();
	
	Pip::Unload(QDIR_FILE);
#ifdef __PLAT_NGC__
	switch ( Config::GetLanguage() )
	{
		case Config::LANGUAGE_FRENCH:
			Pip::UnloadPre("qb.prf");
			break;
		case Config::LANGUAGE_GERMAN:
			Pip::UnloadPre("qb.prd");
			break;
		default:
			Pip::UnloadPre("qb.pre");
			break;
	}
#else
	Pip::UnloadPre("qb.pre");
#endif		// __PLAT_NGC__
	
	// A very small memory optimization ...
	// Remove the debugger_names array since it only exists to get its checksum names registered
	// for the script debugger. They will have got registered by now, so the array is no longer
	// needed. Saves about 3K.
	CSymbolTableEntry *p_debugger_names=LookUpSymbol(CRCD(0xc2f97ba7,"debugger_names"));
	if (p_debugger_names)
	{
		CleanUpAndRemoveSymbol(p_debugger_names);
	}
}

// Calls the Script::LoadQB, and then do any game-specific stuff that needs to be done when a qb is reloaded, such as 
// generating the node name hash table & prefix info
void LoadQB(const char *p_fileName, EBoolAssertIfDuplicateSymbols assertIfDuplicateSymbols)
{
	Dbg_MsgAssert(p_fileName,("NULL p_fileName"));
	//if (strcmp("..\\runnow.qb",p_fileName) != 0)
	//{
	//	printf("Loading %s\n",p_fileName); // REMOVE
	//}
	// Call the non-game-specific LoadQB, which open the qb and create all the symbols defined within.
	Script::LoadQB(p_fileName, assertIfDuplicateSymbols);
	
	CSymbolTableEntry *p_sym=GetNextSymbolTableEntry();
	while (p_sym)
	{
		if (p_sym->mGotReloaded && 
			p_sym->mType==ESYMBOLTYPE_ARRAY &&
			p_sym->mNameChecksum==0xc472ecc5/*NodeArray*/)
		{
			// This will resize the NodeArray, adding the extra nodes required by the goal editor.
			// Note that p_sym->mpArray is still valid after doing this, because the resizing only
			// causes a reallocation of a buffer inside the CArray, the CArray itself is not reallocated.
			Obj::InsertGoalEditorNodes();
			
			CreateNodeNameHashTable();
			GeneratePrefixInfo();

			// Do a bit of node array processing.
			int size=p_sym->mpArray->GetSize();
			for (int i=0; i<size; ++i)
			{
				CStruct *p_node=p_sym->mpArray->GetStructure(i);
				
				// K: This swapping of Position for Pos used to be at a lower level in parse.cpp,
				// but that had the problem that it was doing the swap on all structures, including those
				// passed to commands in scripts. So I moved it here so as to only affect those structures
				// in the node array.
				Mth::Vector p(0.0f,0.0f,0.0f);
				if (p_node->GetVector(CRCD(0xb9d31b0a,"Position"),&p))
				{
					// GJ:  The "position" currently written out 
					// by the exporter has an incorrect Z, and there
					// are a bunch of places in the game code which
					// negate the Z value in order to correct it.
					// We're currently in the process of phasing this
					// "Position" out, in favor of the "Pos" vector, 
					// which will have the correct Z.  For backwards
					// compatibility, the code here removes the old
					// "Position" vector if it exists and replaces 
					// it with the new "Pos" vector.  Eventually, 
					// once all the levels have been re-exported, 
					// we can remove this kludge.
					p_node->RemoveComponent(CRCD(0xb9d31b0a,"Position"));
					p_node->AddVector(CRCD(0x7f261953,"Pos"),p[X],p[Y],-p[Z]);
				}	
			}
			
			// Do some path preprocessing. This adds PathNum parameters to all nodes that
			// are on a path so that one can quickly find out what path a particular car is
			// on without having to scan through a bunch of nodes.
			Obj::CPathMan::Instance()->AddPathInfoToNodeArray();
			
			// Reset the reloaded flag, otherwise it will stick on forever.
			p_sym->mGotReloaded=false;
			
			// No need to keep checking for more NodeArray's, there can be only one.
			break;
		}
		p_sym=GetNextSymbolTableEntry(p_sym);
	}		
	
	// Make sure checksum name lookup table is removed since it is only needed for the node array.
	RemoveChecksumNameLookupHashTable();
}

// Calls the Script::UnloadQB and then does any game-specific stuff that needs to be done 
// when a qb is unloaded, such as resetting the node name hash table & prefix info.
void UnloadQB(uint32 fileNameChecksum)
{
	// Call the non-game-specific UnloadQB, which will delete all the symbols that came from this qb.
	Script::UnloadQB(fileNameChecksum);
	
	if (!GetArray(0xc472ecc5/*NodeArray*/))
	{
		// If there is no node array any more then make sure the prefix info and node name lookup
		// info is cleaned up.
		DeletePrefixInfo();
		ClearNodeNameHashTable();
	}	
}

uint32 GenerateFileNameChecksum(const char *p_fileName)
{
	Dbg_MsgAssert(p_fileName,("NULL p_fileName"));
	
	// TODO May need to process the file name a bit first, eg prefix it with the
	// full path if it is not there already, before calculating its checksum, in order to ensure a match.
	// We may want UnloadQB to be able to be passed a filename without the full path.
	
	// For the moment though, just calculate the checksum for the string as it is.
	return Crc::GenerateCRCFromString(p_fileName);
}

void UnloadQB(const char *p_fileName)
{
	SkateScript::UnloadQB(GenerateFileNameChecksum(p_fileName));
}

// TODO Need to add the old 'DoIt' function which executes when a qbr is done. It does a LoadQB then
// reloads the skater exceptions, updates the decks on the skateshop wall, etc...

} // namespace SkateScript

