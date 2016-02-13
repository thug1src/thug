///////////////////////////////////////////////////////////////////////////////////////
//
// file.cpp		KSH 31 Oct 2001
//
// Functions for loading and unloading qb files.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <core/defines.h>
#include <core/debug.h>
#include <gel/scripting/file.h>
#include <gel/scripting/parse.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/script.h>
#include <gel/scripting/checksum.h>
#include <core/crc.h> // For Crc::GenerateCRCFromString
#include <sys/file/pip.h>

namespace Script
{
// TODO: Need another LoadQB in the game-specific script namespace, which will call this LoadQB
// and then do any game-specific stuff that needs to be done when a qb is reloaded, such as 
// generating the node name hash table & prefix info, reloading the skater exceptions, updating the decks on
// the skateshop wall, etc...

void restart_dirty_scripts()
{
	// For each symbol, check if it got reloaded by the above load, and if it did and it is a script
	// then run through all the existing CScript's restarting those that referred to it.
	// This is essential, because otherwise those CScripts' program counter pointers will now be invalid.
	CSymbolTableEntry *p_sym=GetNextSymbolTableEntry();
	while (p_sym)
	{
		if (p_sym->mGotReloaded && p_sym->mType==ESYMBOLTYPE_QSCRIPT)
		{
			CScript *p_script=GetNextScript();
			while (p_script)
			{
				if (p_script->RefersToScript(p_sym->mNameChecksum))
				{
					p_script->Restart();
				}
			
				p_script=GetNextScript(p_script);
			}	
			
			// Reset the reloaded flag, otherwise it will stick on forever.
			p_sym->mGotReloaded=false;
		}
		p_sym=GetNextSymbolTableEntry(p_sym);
	}		
}

// Loads a QB file.
// It will open the file, load it into memory and parse it, creating all the
// symbols (scripts, arrays, integers etc) defined within in.
void LoadQB(const char *p_fileName, EBoolAssertIfDuplicateSymbols assertIfDuplicateSymbols)
{
	Dbg_MsgAssert(p_fileName,("NULL p_fileName"));
#ifndef __PLAT_NGC__
	Dbg_MsgAssert(strcmp(p_fileName+strlen(p_fileName)-3,".qb")==0,("File does not have extension .qb. File %s",p_fileName));
#endif __PLAT_NGC__

	// Mick - Pip::Load is not going to load it from a Pip::Pre, just a regular pre
	// so I'm sticking it on the top-down heap to avoid fragmentation							  
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	uint8 *p_qb=(uint8*)Pip::Load(p_fileName);
	Mem::Manager::sHandle().PopContext();
		
	// Parse the QB, which creates all the symbols defined within it.
	ParseQB(p_fileName,p_qb,assertIfDuplicateSymbols);
	
	Pip::Unload(p_fileName);	

	restart_dirty_scripts();
}

// Loads a QB file from memory
void LoadQBFromMemory(const char* p_fileName, uint8* p_qb, EBoolAssertIfDuplicateSymbols assertIfDuplicateSymbols)
{
	// even though there's no actually filename,
	// we'd still need a dummy string, which will
	// be used for printing up Assert messages...

	Dbg_MsgAssert(p_fileName,("NULL p_fileName"));
#ifndef __PLAT_NGC__
	Dbg_MsgAssert(strcmp(p_fileName+strlen(p_fileName)-3,".qb")==0,("File does not have extension .qb. File %s",p_fileName));
#endif __PLAT_NGC__

	// do not create the hash table if it's not
	// the node array (it's a 50K memory hog on
	// the script heap, and it's really only
	// needed for doing prefix stuff anyway)
	ParseQB(p_fileName,p_qb,assertIfDuplicateSymbols,false);

	RemoveChecksumNameLookupHashTable();

	restart_dirty_scripts();
}

// TODO: Need another UnloadQB in the game-specific script namespace, which will call this UnloadQB
// and then do any game-specific stuff that needs to be done when a qb is unloaded, such as 
// resetting the node name hash table & prefix info.

// Deletes all the symbols that were defined in the passed QB file.
void UnloadQB(uint32 fileNameChecksum)
{
	// Do nothing if the checksum is zero. This is essential, because symbols
	// that have a mSourceFileNameChecksum of 0 are those that did not come
	// from a qb file, ie C-functions. These must never be deleted.
	if (!fileNameChecksum)
	{
		return;
	}
		
	// Scan through all the symbols in the symbol table.
	CSymbolTableEntry *p_sym=GetNextSymbolTableEntry();
	while (p_sym)
	{											
		if (p_sym->mSourceFileNameChecksum==fileNameChecksum)
		{
			// This symbol was defined in the passed qb file, so remove it.
			CleanUpAndRemoveSymbol(p_sym);
			// Need to start checking from the start of the list again rather than storing
			// a p_next pointer before calling CleanUpAndRemoveSymbol.
			// This is because CleanUpAndRemoveSymbol may cause the symbol above to move down,
			// invalidating p_next.
			// SPEEDOPT Starting again from the start of the list is inefficient though, because
			// it may have to scan through thousands of symbols just to delete one, and may have to
			// delete say a hundred. Optimize later. (Seems fast enough at the moment)
			p_sym=NULL;
		}	
		p_sym=GetNextSymbolTableEntry(p_sym);
	}
	
	// Scan through all the existing CScripts stopping any that are referring to a script
	// that got deleted above.
	CScript *p_script=GetNextScript();
	while (p_script)
	{
		if (!p_script->AllScriptsInCallstackStillExist())
		{
			p_script->ClearScript();
			p_script->ClearEventHandlerTable();
		}
	
		p_script=GetNextScript(p_script);
	}	
}

// Note: There is no Script::UnloadQB(const char *p_fileName)
// This is because we may not want to do a GenerateCRCFromString on the p_fileName as it stands.
// We may want to make sure the file name is prefixed with the complete path before calculating
// the crc.
// Since the path is game specific, the code to do this is in the SkateScript namespace.
// See SkateScript::UnloadQB(const char *p_fileName) and SkateScript::GenerateFileNameChecksum

} // namespace Script

