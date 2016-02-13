///////////////////////////////////////////////////////////////////////////////////////
//
// gs_init.cpp		KSH 7 Nov 2001
//
// Game-specific script system initialisation.
// Also calls the non-game-specific script initialisation.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <sk/scripting/gs_init.h>
#include <sk/scripting/gs_file.h>
#include <sk/scripting/nodearray.h>
#include <sk/scripting/ftables.h>
#include <gel/scripting/init.h>
#include <gel/scripting/parse.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/checksum.h>

namespace SkateScript
{

// Called once on startup.
void Init()
{
	// Do the non-game-specific script system initialisation
	Mem::PushMemProfile("Script::AllocatePools");
	Script::AllocatePools();
	Mem::PopMemProfile();
	
	
	// Now do the game-specific script system initialisation
	Mem::PushMemProfile("InitNodeNameHashTable");
	InitNodeNameHashTable();
	Mem::PopMemProfile();
	

	Mem::PushMemProfile("Registering script functions");
	Script::RegisterCFunctions(CFunctionLookupTable,GetCFunctionLookupTableSize());
	Script::RegisterMemberFunctions(ppMemberFunctionNames,GetNumMemberFunctions());
	Mem::PopMemProfile();
	
}

void Preload()
{
	LoadAllStartupQBFiles();
	
	#ifdef COUNT_USAGE
	CSymbolTableEntry *p_sym=GetNextSymbolTableEntry();
	while (p_sym)
	{
		if (p_sym->mType==ESYMBOLTYPE_QSCRIPT)
		{
			//printf("%s\n",Script::FindChecksumName(p_sym->mNameChecksum));
			
			uint8 *p_token=p_sym->mpScript;
			// Skip over the 4-byte contents checksum.
			p_token+=4;
			
			while (*p_token!=ESCRIPTTOKEN_KEYWORD_ENDSCRIPT) 
			{
				switch (*p_token)
				{
					case ESCRIPTTOKEN_NAME:
					{
						++p_token;
						uint32 name_checksum=Read4Bytes(p_token).mChecksum;
						
						p_token+=4;
			
						// Look up the name to see if it is a cfunction or member function.
						CSymbolTableEntry *p_entry=Resolve(name_checksum);
						if (p_entry)
						{
							//if (name_checksum==0xb0bcbaec)
							//printf("(%s) Type=%s\n",Script::FindChecksumName(name_checksum),Script::GetTypeName(p_entry->mType));
						
							if (p_entry->mType==ESYMBOLTYPE_CFUNCTION)
							{
								++p_entry->mUsage;
								//printf("%s: %s\n",Script::FindChecksumName(p_sym->mNameChecksum),Script::FindChecksumName(name_checksum));
							}
							else if (p_entry->mType==ESYMBOLTYPE_MEMBERFUNCTION)
							{
								++p_entry->mUsage;
							}	
						}		
						break;
					}	
					default:
						p_token=SkipToken(p_token);
						break;
				}
			}	
		}
		p_sym=GetNextSymbolTableEntry(p_sym);
	}		
	
	p_sym=GetNextSymbolTableEntry();
	while (p_sym)
	{
		if (p_sym->mUsage==0)
		{
			if (p_sym->mType==ESYMBOLTYPE_CFUNCTION)
			{
				printf("CFunc %s: Usage=%d\n",Script::FindChecksumName(p_sym->mNameChecksum),p_sym->mUsage);
			}
			if (p_sym->mType==ESYMBOLTYPE_MEMBERFUNCTION)
			{
				//printf("MemberFunc %s: Usage=%d\n",Script::FindChecksumName(p_sym->mNameChecksum),p_sym->mUsage);
			}
		}	
		p_sym=GetNextSymbolTableEntry(p_sym);
	}		
	
	printf("Finished\n");
	while (1);
	#endif // #ifdef COUNT_USAGE



	
	// Now that all the qb's are loaded, pre-process all the scripts.
	//PreProcessScripts();
}

} // namespace SkateScript

