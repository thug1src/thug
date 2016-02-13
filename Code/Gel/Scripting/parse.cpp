///////////////////////////////////////////////////////////////////////////////////////
//
// parse.cpp		KSH 23 Oct 2001
//
// Functions for parsing qb files.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <gel/scripting/parse.h>
#include <gel/scripting/tokens.h>
#include <gel/scripting/component.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/string.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/eval.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/scriptcache.h>
#include <gel/scripting/script.h>
#include <core/math/math.h> // For Mth::Rnd and Mth::Rnd2
#include <core/crc.h> // For Crc::GenerateCRCFromString
#include <core/compress.h>

#ifdef __PLAT_NGC__
#include <sys/ngc/p_aram.h>
#include <sys/ngc/p_dma.h>
#endif		// __PLAT_NGC__

DefinePoolableClass(Script::CStoredRandom);

namespace Script
{

class CScript;
// Declaring this seperately rather than including script.h to avoid a cyclic dependency.
// The full class declaration of CScript is not required.
extern CScript *GetCurrentScript();

static uint8 *sCalculateRandomRange(uint8 *p_token, CComponent *p_comp, bool useRnd2);
static uint8 *sSkipType(uint8 *p_token);
static bool   sIsEndOfLine(const uint8 *p_token);
static uint8 *sInitArrayFromQB(CArray *p_dest, uint8 *p_token, CStruct *p_args=NULL);
static uint8 *sAddComponentFromQB(CStruct *p_dest, uint32 nameChecksum, uint8 *p_token, CStruct *p_args=NULL);
static uint8 *sAddComponentsWithinCurlyBraces(CStruct *p_dest, uint8 *p_token, CStruct *p_args=NULL);
static CSymbolTableEntry *sCreateScriptSymbol(uint32 nameChecksum, uint32 contentsChecksum, const uint8 *p_data, uint32 size, const char *p_fileName);
static uint8 *sCreateSymbolOfTheFormNameEqualsValue(uint8 *p_token, const char *p_fileName, EBoolAssertIfDuplicateSymbols assertIfDuplicateSymbols);
static CStoredRandom *sFindStoredRandom(const uint8 *p_token, EScriptToken type, int numItems);
static CStoredRandom *sCreateNewStoredRandom();

static CStoredRandom *sp_first_stored_random=NULL;
static CStoredRandom *sp_last_stored_random=NULL;
static int s_num_stored_randoms=0;

static uint32 s_qb_being_parsed=0;

// The SkipToken function is in skiptoken.cpp, so that it can also be included in PC code,
// such as qcomp.
#include <gel/scripting/skiptoken.cpp>

#ifdef __NOPT_ASSERT__
void CheckForPossibleInfiniteLoops(uint32 scriptName, uint8 *p_token, const char *p_fileName)
{
	#define MAX_NESTED_LOOPS 100
	bool loop_is_ok[MAX_NESTED_LOOPS];
	int loop_index=0;
	for (int i=0; i<MAX_NESTED_LOOPS; ++i)
	{
		loop_is_ok[i]=false;
	}	

	Script::CArray *p_ok_scripts=Script::GetArray(CRCD(0x8b860fef,"DoNotAssertForInfiniteLoopsInTheseScripts"));
	Script::CArray *p_blocking_functions=Script::GetArray(CRCD(0x215d13b9,"BlockingFunctions"));
	
			
	while (*p_token!=ESCRIPTTOKEN_KEYWORD_ENDSCRIPT) 
	{
		switch (*p_token)
		{
			case ESCRIPTTOKEN_KEYWORD_BEGIN:
				loop_is_ok[loop_index++]=false; // It's guilty until proven innocent!
				Dbg_MsgAssert(loop_index<=100,("Too many nested loops"));
				p_token=SkipToken(p_token);
				break;
			case ESCRIPTTOKEN_KEYWORD_BREAK:
			{
				Dbg_MsgAssert(loop_index,("Zero loop_index"));
				int i=loop_index-1;
				while (i>=0)
				{
					loop_is_ok[i]=true;
					--i;
				}	
				p_token=SkipToken(p_token);
				break;
			}	
			case ESCRIPTTOKEN_KEYWORD_REPEAT:
				p_token=SkipToken(p_token);
				
				Dbg_MsgAssert(loop_index,("Zero loop_index"));
				if (*p_token==ESCRIPTTOKEN_ENDOFLINE || *p_token==ESCRIPTTOKEN_ENDOFLINENUMBER)
				{
					if (!loop_is_ok[loop_index-1])
					{
						bool allow=false;
						if (p_ok_scripts)
						{
							for (uint32 i=0; i<p_ok_scripts->GetSize(); ++i)
							{
								if (p_ok_scripts->GetChecksum(i)==scriptName)
								{
									allow=true;
								}
							}
						}
						
						if (allow)
						{
						}
						else
						{
							Dbg_MsgAssert(0, ("Warning! Possible infinite loop: Line %d of %s\n",Script::GetLineNumber(p_token),p_fileName));
						}	
					}
				}
				else
				{
					// If repeat is not followed by an end-of-line then it is probably followed by some
					// count value, so it is not an infinite loop.
				}
				
				--loop_index;	
				break;

			case ESCRIPTTOKEN_NAME:
			{
				++p_token;
				uint32 name=Read4Bytes(p_token).mChecksum;
				p_token+=4;
				
				if (p_blocking_functions)
				{
					uint32 *p_function_names=p_blocking_functions->GetArrayPointer();
					uint32 size=p_blocking_functions->GetSize();
					
					for (uint32 i=0; i<size; ++i)
					{
						if (name == *p_function_names++)
						{
							if (loop_index)
							{
								int i=loop_index-1;
								while (i>=0)
								{
									loop_is_ok[i]=true;
									--i;
								}	
							}	
							break;
						}
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
#endif

// Used for evaluating the ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE operator.
static uint8 *sCalculateRandomRange(uint8 *p_token, CComponent *p_comp, bool useRnd2)
{
	// RandomRange token must be followed by a pair.
	Dbg_MsgAssert(*p_token==ESCRIPTTOKEN_PAIR,("RandomRange operator must be followed by a pair of values in parentheses, File %s, line %d",GetSourceFile(p_token),GetLineNumber(p_token)));
	++p_token;
	// Get the range values, which are floats cos it's a pair.
	float x=Read4Bytes(p_token).mFloat;
	p_token+=4;
	float y=Read4Bytes(p_token).mFloat;
	p_token+=4;
	
	// Get the integer values.
	// Offset the floats up or down by 0.01 to ensure they round down to the expected integer values.
	int a, b;
	a=(int)( (x<0) ? x-0.1f:x+0.1f);
	b=(int)( (y<0) ? y-0.1f:y+0.1f);
	
	bool they_are_integers=true;
	if (fabs(x-a) > 0.00001f || fabs(y-b) > 0.00001f)
	{
		they_are_integers=false;
	}	
	
	if (they_are_integers)
	{
		// Make sure that a<=b
		if (a>b)
		{
			// Swap them.
			int temp=a;
			a=b;
			b=temp;
		}	
		// Get val, which is between a and b inclusive.
		// This assert is to make sure the calculations in Rnd don't overflow.
		Dbg_MsgAssert(b-a+1<=32767,("File %s, line %d: RandomRange limits are too far apart, max is 32767",GetSourceFile(p_token),GetLineNumber(p_token)));
		int val;
		if (useRnd2)
		{
			val=a+Mth::Rnd2(b-a+1);
		}
		else
		{
			val=a+Mth::Rnd(b-a+1);
		}	
		// Make totally sure the value is OK, cos could lead to hard-to-track-down bugs otherwise.
//		Dbg_MsgAssert(val>=a && val<=b,("File %s, line %d: Internal error in RandomRange (%f %f %f), fire Ken.",GetSourceFile(p_token),GetLineNumber(p_token),x,y,val));
		
		p_comp->mType=ESYMBOLTYPE_INTEGER;
		p_comp->mIntegerValue=val;
	}
	else
	{
		// Make sure that x<=y
		if (x>y)
		{
			// Swap them.
			float temp=x;
			x=y;
			y=temp;
		}	
		// Get val, which is between x and y inclusive.
		float val;
		// Don't make FLOAT_RES bigger than 32766 because the calculations in Rnd will overflow otherwise.
		#define FLOAT_RES 32760.0f
		if (useRnd2)
		{
			val=x+((float)Mth::Rnd2((int)(FLOAT_RES+1.0f))*(y-x))/FLOAT_RES;
		}
		else
		{
			val=x+((float)Mth::Rnd((int)(FLOAT_RES+1.0f))*(y-x))/FLOAT_RES;
		}	
		
		// Make totally sure the value is OK, cos could lead to hard-to-track-down bugs otherwise.
//		Dbg_MsgAssert(val>=x && val<=y,("File %s, line %d: Internal error in RandomRange (%f %f %f), fire Ken.",GetSourceFile(p_token),GetLineNumber(p_token),x,y,val));
		
		p_comp->mType=ESYMBOLTYPE_FLOAT;
		p_comp->mFloatValue=val;
	}
	
	return p_token;
}

// Skips over a data type token. Eg, if p_token points to a structure, it will
// skip over the whole structure.
static uint8 *sSkipType(uint8 *p_token)
{
    switch (*p_token)
    {
        case ESCRIPTTOKEN_NAME:
        case ESCRIPTTOKEN_INTEGER:
        case ESCRIPTTOKEN_HEXINTEGER:
        case ESCRIPTTOKEN_FLOAT:
            p_token+=1+4;
            break;
        case ESCRIPTTOKEN_PAIR:
            p_token+=1+2*4;
            break;
        case ESCRIPTTOKEN_VECTOR:
            p_token+=1+3*4;
            break;
        case ESCRIPTTOKEN_STRING:
        case ESCRIPTTOKEN_LOCALSTRING:
            p_token+=1+4+Read4Bytes(p_token+1).mUInt;
            break;
        case ESCRIPTTOKEN_STARTSTRUCT:
		{
            int StructCount=1;
            while (StructCount)
            {
                p_token=SkipToken(p_token);
                if (*p_token==ESCRIPTTOKEN_STARTSTRUCT) 
				{
					++StructCount;
				}	
                else if (*p_token==ESCRIPTTOKEN_ENDSTRUCT)
				{
                    --StructCount;
				}	
            }
            ++p_token;
            break;
		}	
        case ESCRIPTTOKEN_STARTARRAY:
		{
            int ArrayCount=1;
            while (ArrayCount)
            {
                p_token=SkipToken(p_token);
                if (*p_token==ESCRIPTTOKEN_STARTARRAY)
				{
                    ++ArrayCount;
				}	
                else if (*p_token==ESCRIPTTOKEN_ENDARRAY)
				{
                    --ArrayCount;
				}	
            }
            ++p_token;
            break;
		}	
        default:
            Dbg_MsgAssert(0,("Unrecognized type"));
            break;
    }
    return p_token;
}

// Returns true if pToken points to and end-of-line, end-of-line-number or end-of-file token.
static bool sIsEndOfLine(const uint8 *p_token)
{
	Dbg_MsgAssert(p_token,("NULL p_token"));
    return (*p_token==ESCRIPTTOKEN_ENDOFLINE || *p_token==ESCRIPTTOKEN_ENDOFLINENUMBER || *p_token==ESCRIPTTOKEN_ENDOFFILE);
}

// Given that p_token points to a ESCRIPTTOKEN_STARTARRAY token, this will parse the
// array tokens adding the elements to the CArray.
// Gives error messages if all the elements are not of the same type.
static uint8 *sInitArrayFromQB(CArray *p_dest, uint8 *p_token, CStruct *p_args)
{
	Dbg_MsgAssert(p_dest,("NULL p_dest"));
	Dbg_MsgAssert(p_token,("NULL p_token"));
    Dbg_MsgAssert(*p_token==ESCRIPTTOKEN_STARTARRAY,("p_token does not point to an array"));
	
	// Remember the start, since we're going to do a first pass through to determine the array type and size.
	uint8 *p_start=p_token;	
    
	// Skip over the startarray token.
    ++p_token;
	// Execute any random operators.
	p_token=DoAnyRandomsOrJumps(p_token);

	ESymbolType type=ESYMBOLTYPE_NONE;
	uint32 size=0;
	
    while (*p_token!=ESCRIPTTOKEN_ENDARRAY)
    {
		// Determine type.
        switch (*p_token)
        {
            case ESCRIPTTOKEN_ENDOFLINE:
            case ESCRIPTTOKEN_ENDOFLINENUMBER:
            case ESCRIPTTOKEN_COMMA:
                break;
            case ESCRIPTTOKEN_NAME:
                type=ESYMBOLTYPE_NAME;
                break;
            case ESCRIPTTOKEN_INTEGER:
            case ESCRIPTTOKEN_HEXINTEGER:
				// Integers don't override floats.
                if (type!=ESYMBOLTYPE_FLOAT) 
				{
					type=ESYMBOLTYPE_INTEGER;
				}	
                break;
            case ESCRIPTTOKEN_FLOAT:
                type=ESYMBOLTYPE_FLOAT;
                break;
            case ESCRIPTTOKEN_PAIR:
                type=ESYMBOLTYPE_PAIR;
                break;
            case ESCRIPTTOKEN_VECTOR:
                type=ESYMBOLTYPE_VECTOR;
                break;
	        case ESCRIPTTOKEN_STRING:
                type=ESYMBOLTYPE_STRING;
                break;
            case ESCRIPTTOKEN_LOCALSTRING:
                type=ESYMBOLTYPE_LOCALSTRING;
                break;
            case ESCRIPTTOKEN_STARTSTRUCT:
                type=ESYMBOLTYPE_STRUCTURE;
                break;
            case ESCRIPTTOKEN_STARTARRAY:
                type=ESYMBOLTYPE_ARRAY;
                break;
            default:
                Dbg_MsgAssert(0,("Unrecognized data type in array, File %s, line %d\n",GetSourceFile(p_token),GetLineNumber(p_token)));
                break;
        }
		
		// Update the size and advance p_token.
        switch (*p_token)
        {
            case ESCRIPTTOKEN_ENDOFLINE:
            case ESCRIPTTOKEN_ENDOFLINENUMBER:
            case ESCRIPTTOKEN_COMMA:
                p_token=SkipToken(p_token);
                break;
            case ESCRIPTTOKEN_NAME:
            case ESCRIPTTOKEN_INTEGER:
            case ESCRIPTTOKEN_HEXINTEGER:
            case ESCRIPTTOKEN_FLOAT:
            case ESCRIPTTOKEN_PAIR:
            case ESCRIPTTOKEN_VECTOR:
	        case ESCRIPTTOKEN_STRING:
            case ESCRIPTTOKEN_LOCALSTRING:
            case ESCRIPTTOKEN_STARTSTRUCT:
            case ESCRIPTTOKEN_STARTARRAY:
                ++size;
                p_token=sSkipType(p_token);
                break;
            default:
                Dbg_MsgAssert(0,("Unrecognized data type in array, File %s, line %d\n",GetSourceFile(p_token),GetLineNumber(p_token)));
                break;
        }
		
		// Execute any random operators. This has to be done each time p_token is advanced.
		p_token=DoAnyRandomsOrJumps(p_token);
    }

	if (type==ESYMBOLTYPE_NONE)
	{
		// Skip over the ESCRIPTTOKEN_ENDARRAY
		++p_token;
		// Finished, empty array.
		return p_token;
	}

	// Rewind.										  
	p_token=p_start;	
	
	// Now that the array size and type are known, set up the CArray and fill it in.
	
	// Make sure we're using the script heap, because the CArray is not hard-wired to
	// allocate it's buffer off it.
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
	p_dest->SetSizeAndType(size,type);
	Mem::Manager::sHandle().PopContext();

	// Just to be totally sure, cos we're about to write into it ...
	Dbg_MsgAssert(p_dest->GetArrayPointer(),("NULL array pinter ???"));
	
	#ifdef __NOPT_ASSERT__
	int size_check=size;
	#endif
		
    switch (type)
    {
		case ESYMBOLTYPE_INTEGER:
		{
			int *p_int=(int*)p_dest->GetArrayPointer();
			
			// Skip over the ESCRIPTTOKEN_STARTARRAY
			++p_token;
			// Execute any random operators. This has to be done each time p_token is advanced.
			p_token=DoAnyRandomsOrJumps(p_token);
			while (*p_token!=ESCRIPTTOKEN_ENDARRAY)
			{
				switch (*p_token++)
				{
					case ESCRIPTTOKEN_ENDOFLINE:
					case ESCRIPTTOKEN_COMMA:
						break;
					case ESCRIPTTOKEN_ENDOFLINENUMBER:
						p_token+=4;
						break;
					case ESCRIPTTOKEN_INTEGER:
					case ESCRIPTTOKEN_HEXINTEGER:
						#ifdef __NOPT_ASSERT__
						Dbg_MsgAssert(size_check,("Array size mismatch"));
						--size_check;
						#endif
						
						*p_int++=Read4Bytes(p_token).mInt;
						p_token+=4;
						break;
					default:
						Dbg_MsgAssert(0,("Array elements must be of the same type, File %s, line %d\n",GetSourceFile(p_token-1),GetLineNumber(p_token-1)));
						break;
				}
				// Execute any random operators. This has to be done each time p_token is advanced.
				p_token=DoAnyRandomsOrJumps(p_token);
			}    
			#ifdef __NOPT_ASSERT__
			Dbg_MsgAssert(size_check==0,("Array size mismatch"));
			#endif
			// Skip over the ESCRIPTTOKEN_ENDARRAY                                     
			++p_token;
			break;
		}    
		case ESYMBOLTYPE_FLOAT:
		{
			float *p_float=(float*)p_dest->GetArrayPointer();
			
			// Skip over the ESCRIPTTOKEN_STARTARRAY
			++p_token;
			p_token=DoAnyRandomsOrJumps(p_token);
			while (*p_token!=ESCRIPTTOKEN_ENDARRAY)
			{
				switch (*p_token++)
				{
					case ESCRIPTTOKEN_ENDOFLINE:
					case ESCRIPTTOKEN_COMMA:
						break;
					case ESCRIPTTOKEN_ENDOFLINENUMBER:
						p_token+=4;
						break;
					case ESCRIPTTOKEN_INTEGER:
					case ESCRIPTTOKEN_HEXINTEGER:
						#ifdef __NOPT_ASSERT__
						Dbg_MsgAssert(size_check,("Array size mismatch"));
						--size_check;
						#endif
						
						*p_float++=Read4Bytes(p_token).mInt;
						p_token+=4;
						break;
					case ESCRIPTTOKEN_FLOAT:
						#ifdef __NOPT_ASSERT__
						Dbg_MsgAssert(size_check,("Array size mismatch"));
						--size_check;
						#endif
						
						*p_float++=Read4Bytes(p_token).mFloat;
						p_token+=4;
						break;
					default:
						Dbg_MsgAssert(0,("Array elements must be of the same type, File %s, line %d\n",GetSourceFile(p_token-1),GetLineNumber(p_token-1)));
						break;
				}
				// Execute any random operators. This has to be done each time p_token is advanced.
				p_token=DoAnyRandomsOrJumps(p_token);
			}    
			#ifdef __NOPT_ASSERT__
			Dbg_MsgAssert(size_check==0,("Array size mismatch"));
			#endif
			// Skip over the ESCRIPTTOKEN_ENDARRAY                                     
			++p_token;
			break;
		}    
		case ESYMBOLTYPE_NAME:
		{
			uint32 *p_checksum=(uint32*)p_dest->GetArrayPointer();
			
			// Skip over the ESCRIPTTOKEN_STARTARRAY
			++p_token;
			p_token=DoAnyRandomsOrJumps(p_token);
			while (*p_token!=ESCRIPTTOKEN_ENDARRAY)
			{
				switch (*p_token++)
				{
					case ESCRIPTTOKEN_ENDOFLINE:
					case ESCRIPTTOKEN_COMMA:
						break;
					case ESCRIPTTOKEN_ENDOFLINENUMBER:
						p_token+=4;
						break;
					case ESCRIPTTOKEN_NAME:
						#ifdef __NOPT_ASSERT__
						Dbg_MsgAssert(size_check,("Array size mismatch"));
						--size_check;
						#endif
						
						*p_checksum++=Read4Bytes(p_token).mChecksum;
						p_token+=4;
						break;
					default:
						Dbg_MsgAssert(0,("Array elements must be of the same type, File %s, line %d\n",GetSourceFile(p_token-1),GetLineNumber(p_token-1)));
						break;
				}
				// Execute any random operators. This has to be done each time p_token is advanced.
				p_token=DoAnyRandomsOrJumps(p_token);
			}    
			#ifdef __NOPT_ASSERT__
			Dbg_MsgAssert(size_check==0,("Array size mismatch"));
			#endif
			// Skip over the ESCRIPTTOKEN_ENDARRAY                                     
			++p_token;
			break;
		}    
		case ESYMBOLTYPE_STRUCTURE:
		{
			CStruct **pp_structures=(CStruct**)p_dest->GetArrayPointer();

			// For finding out which node in Chad's qn is causing a syntax error
			//int index=0; // REMOVE!
						
			// Skip over the ESCRIPTTOKEN_STARTARRAY
			++p_token;
			p_token=DoAnyRandomsOrJumps(p_token);
			while (*p_token!=ESCRIPTTOKEN_ENDARRAY)
			{
				switch (*p_token)
				{
					case ESCRIPTTOKEN_ENDOFLINE:
					case ESCRIPTTOKEN_COMMA:
						++p_token;
						break;
					case ESCRIPTTOKEN_ENDOFLINENUMBER:
						p_token+=5;
						break;
					case ESCRIPTTOKEN_STARTSTRUCT:
					{
						#ifdef __NOPT_ASSERT__
						Dbg_MsgAssert(size_check,("Array size mismatch"));
						--size_check;
						#endif
						
						// Each structure is individually allocated.
						CStruct *p_struct=new CStruct;
						p_token=sAddComponentsWithinCurlyBraces(p_struct,p_token,p_args);
						
						*pp_structures++=p_struct;
						
						//printf("Created array structure %d\n",index++); // REMOVE!
						
						break;
					}    
					default:
						Dbg_MsgAssert(0,("Array elements must be of the same type, File %s, line %d\n",GetSourceFile(p_token),GetLineNumber(p_token)));
						break;
				}
				// Execute any random operators. This has to be done each time p_token is advanced.
				p_token=DoAnyRandomsOrJumps(p_token);
			}    
			#ifdef __NOPT_ASSERT__
			Dbg_MsgAssert(size_check==0,("Array size mismatch"));
			#endif
			// Skip over the ESCRIPTTOKEN_ENDARRAY                                     
			++p_token;
			break;
		}    
		case ESYMBOLTYPE_ARRAY:
		{
			CArray **pp_arrays=(CArray**)p_dest->GetArrayPointer();
			
			// Skip over the ESCRIPTTOKEN_STARTARRAY
			++p_token;
			p_token=DoAnyRandomsOrJumps(p_token);
			while (*p_token!=ESCRIPTTOKEN_ENDARRAY)
			{
				switch (*p_token++)
				{
					case ESCRIPTTOKEN_ENDOFLINE:
					case ESCRIPTTOKEN_COMMA:
						break;
					case ESCRIPTTOKEN_ENDOFLINENUMBER:
						p_token+=4;
						break;
					case ESCRIPTTOKEN_STARTARRAY:
					{
						#ifdef __NOPT_ASSERT__
						Dbg_MsgAssert(size_check,("Array size mismatch"));
						--size_check;
						#endif
						
						// Each array is individually allocated.											
						CArray *p_array=new CArray;
						// The -1 is to rewind p_token back to pointing to ESCRIPTTOKEN_STARTARRAY,
						// which sInitArrayFromQB requires.
						p_token=sInitArrayFromQB(p_array,p_token-1,p_args);
							 
						*pp_arrays++=p_array;
						break;
					}	
					default:
						Dbg_MsgAssert(0,("Array elements must be of the same type, File %s, line %d\n",GetSourceFile(p_token-1),GetLineNumber(p_token-1)));
						break;
				}
				// Execute any random operators. This has to be done each time p_token is advanced.
				p_token=DoAnyRandomsOrJumps(p_token);
			}    
			#ifdef __NOPT_ASSERT__
			Dbg_MsgAssert(size_check==0,("Array size mismatch"));
			#endif
			// Skip over the ESCRIPTTOKEN_ENDARRAY                                     
			++p_token;
			break;
		}    

		case ESYMBOLTYPE_STRING:
		{
			char **pp_strings=(char**)p_dest->GetArrayPointer();
			
			// Skip over the ESCRIPTTOKEN_STARTARRAY
			++p_token;
			p_token=DoAnyRandomsOrJumps(p_token);
			while (*p_token!=ESCRIPTTOKEN_ENDARRAY)
			{
				switch (*p_token++)
				{
					case ESCRIPTTOKEN_ENDOFLINE:
					case ESCRIPTTOKEN_COMMA:
						break;
					case ESCRIPTTOKEN_ENDOFLINENUMBER:
						p_token+=4;
						break;
					case ESCRIPTTOKEN_STRING:
					{
						#ifdef __NOPT_ASSERT__
						Dbg_MsgAssert(size_check,("Array size mismatch"));
						--size_check;
						#endif

						int string_length=Read4Bytes(p_token).mInt;
						Dbg_MsgAssert(string_length,("Zero string_length?"));
						p_token+=4;
						
						*pp_strings++=CreateString((const char *)p_token);
						p_token+=string_length;
						break;
					}	
					default:
						Dbg_MsgAssert(0,("Array elements must be of the same type, File %s, line %d\n",GetSourceFile(p_token-1),GetLineNumber(p_token-1)));
						break;
				}
				// Execute any random operators. This has to be done each time p_token is advanced.
				p_token=DoAnyRandomsOrJumps(p_token);
			}    
			#ifdef __NOPT_ASSERT__
			Dbg_MsgAssert(size_check==0,("Array size mismatch"));
			#endif
			// Skip over the ESCRIPTTOKEN_ENDARRAY                                     
			++p_token;
			break;
		}    

		case ESYMBOLTYPE_LOCALSTRING:
		{
			char **pp_strings=(char**)p_dest->GetArrayPointer();
			
			// Skip over the ESCRIPTTOKEN_STARTARRAY
			++p_token;
			p_token=DoAnyRandomsOrJumps(p_token);
			while (*p_token!=ESCRIPTTOKEN_ENDARRAY)
			{
				switch (*p_token++)
				{
					case ESCRIPTTOKEN_ENDOFLINE:
					case ESCRIPTTOKEN_COMMA:
						break;
					case ESCRIPTTOKEN_ENDOFLINENUMBER:
						p_token+=4;
						break;
					case ESCRIPTTOKEN_LOCALSTRING:
					{
						#ifdef __NOPT_ASSERT__
						Dbg_MsgAssert(size_check,("Array size mismatch"));
						--size_check;
						#endif

						int string_length=Read4Bytes(p_token).mInt;
						Dbg_MsgAssert(string_length,("Zero string_length?"));
						p_token+=4;
						
						*pp_strings++=CreateString((const char *)p_token);
						p_token+=string_length;
						break;
					}	
					default:
						Dbg_MsgAssert(0,("Array elements must be of the same type, File %s, line %d\n",GetSourceFile(p_token-1),GetLineNumber(p_token-1)));
						break;
				}
				// Execute any random operators. This has to be done each time p_token is advanced.
				p_token=DoAnyRandomsOrJumps(p_token);
			}    
			#ifdef __NOPT_ASSERT__
			Dbg_MsgAssert(size_check==0,("Array size mismatch"));
			#endif
			// Skip over the ESCRIPTTOKEN_ENDARRAY                                     
			++p_token;
			break;
		}    

		case ESYMBOLTYPE_VECTOR:
		{
			CVector **pp_vectors=(CVector**)p_dest->GetArrayPointer();
			
			// Skip over the ESCRIPTTOKEN_STARTARRAY
			++p_token;
			p_token=DoAnyRandomsOrJumps(p_token);
			while (*p_token!=ESCRIPTTOKEN_ENDARRAY)
			{
				switch (*p_token++)
				{
					case ESCRIPTTOKEN_ENDOFLINE:
					case ESCRIPTTOKEN_COMMA:
						break;
					case ESCRIPTTOKEN_ENDOFLINENUMBER:
						p_token+=4;
						break;
					case ESCRIPTTOKEN_VECTOR:
					{
						#ifdef __NOPT_ASSERT__
						Dbg_MsgAssert(size_check,("Array size mismatch"));
						--size_check;
						#endif
						
						CVector *p_new_vector=new CVector;
						p_new_vector->mX=Read4Bytes(p_token).mFloat;
						p_token+=4;
						p_new_vector->mY=Read4Bytes(p_token).mFloat;
						p_token+=4;
						p_new_vector->mZ=Read4Bytes(p_token).mFloat;
						p_token+=4;
						
						*pp_vectors++=p_new_vector;
						break;
					}	
					default:
						Dbg_MsgAssert(0,("Array elements must be of the same type, File %s, line %d\n",GetSourceFile(p_token-1),GetLineNumber(p_token-1)));
						break;
				}
				// Execute any random operators. This has to be done each time p_token is advanced.
				p_token=DoAnyRandomsOrJumps(p_token);
			}    
			#ifdef __NOPT_ASSERT__
			Dbg_MsgAssert(size_check==0,("Array size mismatch"));
			#endif
			// Skip over the ESCRIPTTOKEN_ENDARRAY                                     
			++p_token;
			break;
		}    

		case ESYMBOLTYPE_PAIR:
		{
			CPair **pp_pairs=(CPair**)p_dest->GetArrayPointer();
			
			// Skip over the ESCRIPTTOKEN_STARTARRAY
			++p_token;
			p_token=DoAnyRandomsOrJumps(p_token);
			while (*p_token!=ESCRIPTTOKEN_ENDARRAY)
			{
				switch (*p_token++)
				{
					case ESCRIPTTOKEN_ENDOFLINE:
					case ESCRIPTTOKEN_COMMA:
						break;
					case ESCRIPTTOKEN_ENDOFLINENUMBER:
						p_token+=4;
						break;
					case ESCRIPTTOKEN_PAIR:
					{
						#ifdef __NOPT_ASSERT__
						Dbg_MsgAssert(size_check,("Array size mismatch"));
						--size_check;
						#endif
						
						CPair *p_new_pair=new CPair;
						p_new_pair->mX=Read4Bytes(p_token).mFloat;
						p_token+=4;
						p_new_pair->mY=Read4Bytes(p_token).mFloat;
						p_token+=4;
						
						*pp_pairs++=p_new_pair;
						break;
					}	
					default:
						Dbg_MsgAssert(0,("Array elements must be of the same type, File %s, line %d\n",GetSourceFile(p_token-1),GetLineNumber(p_token-1)));
						break;
				}
				// Execute any random operators. This has to be done each time p_token is advanced.
				p_token=DoAnyRandomsOrJumps(p_token);
			}    
			#ifdef __NOPT_ASSERT__
			Dbg_MsgAssert(size_check==0,("Array size mismatch"));
			#endif
			// Skip over the ESCRIPTTOKEN_ENDARRAY                                     
			++p_token;
			break;
		}    
		
		default:
			Dbg_MsgAssert(0,("Unsupported array type, File %s, line %d\n",GetSourceFile(p_token),GetLineNumber(p_token)));
			break;
    }

	return p_token;
}

// Adds one component to p_dest.
// The component is given the name nameChecksum, and it's type and value is defined by whatever
// p_token is pointing to.
// p_args contains the structure defining the value of any args referred to using the <,> operators.
// Returns a pointer to the next token after parsing the value pointed to by p_token.
static uint8 *sAddComponentFromQB(CStruct *p_dest, uint32 nameChecksum, uint8 *p_token, CStruct *p_args)
{
	Dbg_MsgAssert(p_dest,("NULL p_dest"));
	Dbg_MsgAssert(p_token,("NULL p_token"));
	
	bool use_rnd2=false;
	
	switch (*p_token++)
	{
		case ESCRIPTTOKEN_NAME:
		{
			uint32 checksum=Read4Bytes(p_token).mChecksum;
			p_token+=4;
			p_dest->AddChecksum(nameChecksum,checksum);
			break;
		}    
		case ESCRIPTTOKEN_INTEGER:
		{
			int integer=Read4Bytes(p_token).mInt;
			p_token+=4;
			p_dest->AddInteger(nameChecksum,integer);
			break;
		}    
		case ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE2:
			use_rnd2=true;
			// Intentional lack of a break here.
		case ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE:
		{
			CComponent *p_new_component=new CComponent;
			p_token=sCalculateRandomRange(p_token,p_new_component,use_rnd2);
			p_new_component->mNameChecksum=nameChecksum;
			p_dest->AddComponent(p_new_component);
			break;
		}    
		
		case ESCRIPTTOKEN_FLOAT:
		{
			float float_val=Read4Bytes(p_token).mFloat;
			p_token+=4;
			p_dest->AddFloat(nameChecksum,float_val);
			break;
		}    
		case ESCRIPTTOKEN_VECTOR:
		{
			float x=Read4Bytes(p_token).mFloat;
			p_token+=4;
			float y=Read4Bytes(p_token).mFloat;
			p_token+=4;
			float z=Read4Bytes(p_token).mFloat;
			p_token+=4;
			
			// Alert! This next 'if' is kind of game specific ...
			// It should probably be the exporter that does not include the angles in the node array if they are 
			// close to zero, rather than having this remove them from all structures.
			// Note: We can't just remove zero angles from the node array after it has loaded, because there
			// is not enough memory to store them initially.
			if (nameChecksum==CRCD(0x9d2d0915,"Angles"))
			{
				// If the vector is close to (0,0,0) then don't bother adding it.
				// This is a memory optimization. All nodes in levels are getting exported with
				// an angles component, which most of the time is (0,0,0). 
				// This will work OK so long as whenever GetVector is used in the code to get the angles, 
				// the vector being loaded into is initialised with (0,0,0).
				// That way, if the vector is not there, it will remain as (0,0,0) as required.
				float dx=x;
				if (dx<0.0f) dx=-dx;
				float dy=y;
				if (dy<0.0f) dy=-dy;
				float dz=z;
				if (dz<0.0f) dz=-dz;
				if (dx<0.01f && dy<0.01f && dz<0.01f)
				{
					break;
				}	
			}
			
			p_dest->AddVector(nameChecksum,x,y,z);
			break;
		}    
		case ESCRIPTTOKEN_PAIR:
		{
			float x=Read4Bytes(p_token).mFloat;
			p_token+=4;
			float y=Read4Bytes(p_token).mFloat;
			p_token+=4;
			p_dest->AddPair(nameChecksum,x,y);
			break;
		}    
		case ESCRIPTTOKEN_STRING:
		{
			int len=Read4Bytes(p_token).mInt;
			p_token+=4;
			p_dest->AddString(nameChecksum,(const char *)p_token);
			p_token+=len;
			break;
		}    
		case ESCRIPTTOKEN_LOCALSTRING:
		{
			int len=Read4Bytes(p_token).mInt;
			p_token+=4;
			p_dest->AddLocalString(nameChecksum,(const char *)p_token);
			p_token+=len;
			break;
		}    
		case ESCRIPTTOKEN_STARTSTRUCT:
		{
			// Rewind p_token to point to ESCRIPTTOKEN_STARTSTRUCT, because the 
			// sAddComponentsWithinCurlyBraces call requires it.
			--p_token;
			
			// No need to set which heap we're using, cos CStruct's and their CComponent's come off
			// their own pools.
			CStruct *p_structure=new CStruct;
			p_token=sAddComponentsWithinCurlyBraces(p_structure,p_token,p_args);
			p_dest->AddStructurePointer(nameChecksum,p_structure);
			// Note: Not deleting p_structure, because it has been given to p_dest, which will delete
			// it in its destructor.
			break;
		}    
		case ESCRIPTTOKEN_STARTARRAY:
		{
			// Rewind p_token to point to ESCRIPTTOKEN_STARTARRAY, because the CArray::Parse call requires it.
			--p_token;
			
			// The CArray constructor is not hard-wired to use the script heap for it's buffer, so need
			// to make sure we're using the script heap here.
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
			CArray *p_array=new CArray;
			Mem::Manager::sHandle().PopContext();			
			
			p_token=sInitArrayFromQB(p_array,p_token,p_args);
			p_dest->AddArrayPointer(nameChecksum,p_array);
			// Note: Not deleting p_array, because it has been given to p_dest, which will delete
			// it in its destructor.
			break;
		}
		case ESCRIPTTOKEN_KEYWORD_SCRIPT:
		{
			// When adding a local script defined in a structure, we'd expect the nameChecksum passed to
			// this function to be zero, since the name comes after the script keyword.
			Dbg_MsgAssert(nameChecksum==0,("nameChecksum expected to be 0 for a local script ??, File %s, line %d\n",GetSourceFile(p_token-1),GetLineNumber(p_token-1)));
			
			Dbg_MsgAssert(*p_token==ESCRIPTTOKEN_NAME,("\nKeyword 'script' must be followed by a name, File %s, line %d\n",GetSourceFile(p_token-1),GetLineNumber(p_token-1)));
			++p_token; // Skip over the ESCRIPTTOKEN_NAME
			nameChecksum=Read4Bytes(p_token).mChecksum;
			p_token+=4; // Skip over the name checksum
			
			// Skip p_token over the script tokens, and calculate the size.
			const uint8 *p_script=p_token;
			p_token=SkipOverScript(p_token);
			uint32 size=p_token-p_script;
			
			// This will create a new buffer and copy in the script data.
			p_dest->AddScript(nameChecksum,p_script,size);
 			break;
		}	
		case ESCRIPTTOKEN_ARG:
		{
			Dbg_MsgAssert(*p_token==ESCRIPTTOKEN_NAME,("Expected '<' token to be followed by a name, File %s, line %d",GetSourceFile(p_token-1),GetLineNumber(p_token-1)));
			++p_token;
			uint32 arg_checksum=Read4Bytes(p_token).mChecksum;
			p_token+=4;
			if (p_args)
			{
				// Look for a parameter named arg_checksum in p_args, recursing into any referenced structures
				CComponent *p_comp=p_args->FindNamedComponentRecurse(arg_checksum);
				if (p_comp)
				{
					if (nameChecksum==0 && p_comp->mType==ESYMBOLTYPE_STRUCTURE)
					{
						// Trying to add an unnamed structure, in which case it's contents
						// should be merged onto p_dest.
						p_dest->AppendStructure(p_comp->mpStructure);
						// Note: OK to call AppendStructure with NULL, so no need to check mpStructure.
					}
					else
					{
						// Create a copy of the component and add it to p_dest.
						
						CComponent *p_new_component=new CComponent;
						// Note: There is no copy-constructor for CComponent, because that would cause a
						// cyclic dependency between component.cpp/h and struct.cpp/h.
						// Instead, use the CopyComponent function defined in struct.cpp
						CopyComponent(p_new_component,p_comp);
						// Then change it's name to nameChecksum
						p_new_component->mNameChecksum=nameChecksum;
						// And add it to p_dest
						p_dest->AddComponent(p_new_component);
					}	
				}
			}
			// Don't assert if p_args is NULL, cos it will be when this function is used to skip over
			// parameter lists in FindReferences
			break;	
		}
		case ESCRIPTTOKEN_KEYWORD_ALLARGS:
		{
			// Don't assert if p_args is NULL, cos it will be when this function is used to skip over
			// parameter lists in FindReferences
			if (p_args)
			{
				if (nameChecksum)
				{
					// If it's a named structure, copy the passed arguments into a new structure and insert a 
					// pointer to it.
					CStruct *p_structure=new CStruct;
					*p_structure=*p_args;
					p_dest->AddStructurePointer(nameChecksum,p_structure);
				}
				else	
				{
					// Otherwise just merge in the passed parameters.
					// Un-named structures are always considered 'part of' the structure that they are in,
					// eg {x=9 {y=3}} is the same as {x=9 y=3}
					*p_dest+=*p_args;
				}
			}	
			break;	
		}
		case ESCRIPTTOKEN_COMMA:
			break;
		default:
			--p_token;
			Dbg_MsgAssert(0,("\nBad token '%s' encountered when creating component, File %s, line %d",GetTokenName((EScriptToken)*p_token),GetSourceFile(p_token),GetLineNumber(p_token)));
			break;
	}
    return p_token;    
}

// Parses the components in QB format pointed to by p_token, and adds them to p_dest until the 
// close-curly-brace token is reached.
// p_token must initially point to an open-curly-brace token.
// Any end-of-lines in between are ignored.
// For example, p_token may point to: {x=9 foo="Blaa"}
// Used when creating sub-structures and global structures.
// p_args contains the structure defining the value of any args referred to using the <,> operators.
// Returns a pointer to the next token after parsing the tokens pointed to by p_token.
static uint8 *sAddComponentsWithinCurlyBraces(CStruct *p_dest, uint8 *p_token, CStruct *p_args)
{
	Dbg_MsgAssert(p_dest,("NULL p_dest"));
	Dbg_MsgAssert(p_token,("NULL p_token"));
    Dbg_MsgAssert(*p_token==ESCRIPTTOKEN_STARTSTRUCT,("p_token expected to point to ESCRIPTTOKEN_STARTSTRUCT, File %s, line %d",GetSourceFile(p_token),GetLineNumber(p_token)));
	// Skip over the ESCRIPTTOKEN_STARTSTRUCT
    ++p_token;

    while (true)
    {
		p_token=SkipEndOfLines(p_token);
		// Execute any random operators. This has to be done each time p_token is advanced.
		p_token=DoAnyRandomsOrJumps(p_token);
		if (*p_token==ESCRIPTTOKEN_ENDSTRUCT)
		{
			break;
		}	
		
        switch (*p_token)
        {
            case ESCRIPTTOKEN_NAME:
            {
                uint8 *p_name_token=p_token;
                ++p_token;
                uint32 name_checksum=Read4Bytes(p_token).mChecksum;
                p_token+=4;
                p_token=SkipEndOfLines(p_token);
				p_token=DoAnyRandomsOrJumps(p_token);
                if (*p_token==ESCRIPTTOKEN_EQUALS)
                {
                    ++p_token;
                    p_token=SkipEndOfLines(p_token);
					p_token=DoAnyRandomsOrJumps(p_token);
					
					if (*p_token==ESCRIPTTOKEN_OPENPARENTH)
					{
						CComponent *p_comp=new CComponent;
						p_token=Evaluate(p_token,p_args,p_comp);
						if (p_comp->mType!=ESYMBOLTYPE_NONE)
						{
							p_comp->mNameChecksum=name_checksum;
							p_dest->AddComponent(p_comp);
						}
						else
						{
							delete p_comp;
						}	
					}
					else
					{
						p_token=sAddComponentFromQB(p_dest,name_checksum,p_token,p_args);
					}
                }
                else
				{
                    p_token=sAddComponentFromQB(p_dest,NO_NAME,p_name_token,p_args);
				}	
                break;
            }    
            case ESCRIPTTOKEN_STARTSTRUCT:
                p_token=sAddComponentsWithinCurlyBraces(p_dest,p_token,p_args);
                break;
			case ESCRIPTTOKEN_COMMA:	
				++p_token;
				break;
			case ESCRIPTTOKEN_ENDOFLINE:
			case ESCRIPTTOKEN_ENDOFLINENUMBER:
				p_token=SkipEndOfLines(p_token);				
				break;
            default:
			{
				if (*p_token==ESCRIPTTOKEN_OPENPARENTH)
				{
					CComponent *p_comp=new CComponent;
					p_token=Evaluate(p_token,p_args,p_comp);
					
					if (p_comp->mType==ESYMBOLTYPE_STRUCTURE)
					{
						// The CStruct::AddComponent function does not allow unnamed structure
						// components to be added, so merge in the contents of the structure instead.
						p_dest->AppendStructure(p_comp->mpStructure);
						// Now p_comp does have to be cleaned up and deleted, because it has not
						// been given to p_dest.
						CleanUpComponent(p_comp);
						delete p_comp;
					}
					else
					{
						if (p_comp->mType!=ESYMBOLTYPE_NONE)
						{
							p_comp->mNameChecksum=NO_NAME;
							p_dest->AddComponent(p_comp);
						}
						else
						{
							delete p_comp;
						}	
					}				
				}
				else
				{
					p_token=sAddComponentFromQB(p_dest,NO_NAME,p_token,p_args);
				}	
                break;
			}	
        }            
    } 

	// Skip over the ESCRIPTTOKEN_ENDSTRUCT
    ++p_token;
    return p_token;
}

// Creates a new script symbol entry, allocates memory for the script data and copies it in, prefixing
// the data with the contents checksum.
static CSymbolTableEntry *sCreateScriptSymbol(uint32 nameChecksum, uint32 contentsChecksum, const uint8 *p_data, uint32 size, const char *p_fileName)
{
	Dbg_MsgAssert(p_data,("NULL p_data ??"));
	Dbg_MsgAssert(p_fileName,("NULL p_fileName"));

	#ifdef __NOPT_ASSERT__
	#ifndef __PLAT_WN32__
	CheckForPossibleInfiniteLoops(nameChecksum,(uint8*)(uint32)p_data, p_fileName);
	#endif
	#endif

	CSymbolTableEntry *p_new=CreateNewSymbolEntry(nameChecksum);
	Dbg_MsgAssert(p_new,("NULL p_new ??"));
	
	p_new->mType=ESYMBOLTYPE_QSCRIPT;
	
	#ifdef NO_SCRIPT_CACHING
	// Allocate space for the content checksum (4 bytes) plus the script data.
	uint8 *p_new_script=(uint8*)Mem::Malloc(4+size);
	
	// Write in the contents checksum.
	// p_new_script will be long-word aligned so OK to cast to a uint32*
	*(uint32*)p_new_script=contentsChecksum;
	
	uint8 *p_dest=p_new_script+4;
	const uint8 *p_source=p_data;
	for (uint32 i=0; i<size; ++i)
	{
		*p_dest++=*p_source++;
	}	
	p_new->mpScript=p_new_script;
	PreProcessScript(p_new_script+4);
	
	#else	
	
	
	enum
	{
		COMPRESS_BUFFER_SIZE=20000,
	};	
	uint8 *p_compress_buffer=(uint8*)Mem::Malloc(COMPRESS_BUFFER_SIZE);
	
	// Compress the script data.
	int compressed_size=Encode((char*)p_data,(char*)p_compress_buffer,size,false);
	Dbg_MsgAssert(compressed_size <= COMPRESS_BUFFER_SIZE,("Compress buffer overflow! Compressed size of script %s is %d",Script::FindChecksumName(nameChecksum),compressed_size));
	
	// If it compressed to a bigger size, replace the compressed 
	// data with a copy of the original instead.
	if (compressed_size >= (int)size)
	{
		const uint8 *p_source=p_data;
		uint8 *p_dest=p_compress_buffer;
		for (uint32 i=0; i<size; ++i)
		{
			*p_dest++=*p_source++;
		}	
		compressed_size=size;
	}
	
	// Allocate space for the content checksum, decompressed size, compressed size, and the compressed data.
	uint8 *p_new_script=(uint8*)Mem::Malloc(SCRIPT_HEADER_SIZE+compressed_size);

	// p_new_script will be long-word aligned so OK to cast to a uint32*
	*(uint32*)(p_new_script+0)=contentsChecksum;
	*(uint32*)(p_new_script+4)=size;
	*(uint32*)(p_new_script+8)=compressed_size;
	
	uint8 *p_dest=p_new_script+SCRIPT_HEADER_SIZE;
	const uint8 *p_source=p_compress_buffer;
	for (int i=0; i<compressed_size; ++i)
	{
		*p_dest++=*p_source++;
	}	
	
#ifdef __PLAT_NGC__
	p_new->mScriptOffset = NsARAM::alloc( SCRIPT_HEADER_SIZE+compressed_size, NsARAM::SCRIPT );
	NsDMA::toARAM( p_new->mScriptOffset, p_new_script, SCRIPT_HEADER_SIZE+compressed_size );
	Mem::Free( p_new_script );
#else
	p_new->mpScript=p_new_script;
#endif		// __PLAT_NGC__
	
	Mem::Free(p_compress_buffer);
	
	// Now that the new script has been loaded, the script cache needs to be refreshed in case any existing
	// CScript's are running this script. They will all get restarted later (see file.cpp)
	Script::CScriptCache *p_script_cache=Script::CScriptCache::Instance();
	// Not asserting if p_script_cache is NULL, because it will be when all the q-files are loaded on startup.
	if (p_script_cache)	
	{
		p_script_cache->RefreshAfterReload(nameChecksum);
	}
	#endif

	// Store the name of the source qb in the symbol so that the qb is able to be unloaded.
	p_new->mSourceFileNameChecksum=Crc::GenerateCRCFromString(p_fileName);
	
	return p_new;
}

// Creates a symbol defined by a name, followed by equals, followed by a value.
static uint8 *sCreateSymbolOfTheFormNameEqualsValue(uint8 *p_token, const char *p_fileName, EBoolAssertIfDuplicateSymbols assertIfDuplicateSymbols)
{
	Dbg_MsgAssert(p_token,("NULL p_token"));
	Dbg_MsgAssert(p_fileName,("NULL p_fileName"));
	
	Dbg_MsgAssert(*p_token==ESCRIPTTOKEN_NAME,("\nExpected ESCRIPTTOKEN_NAME"));
	++p_token;
	uint32 name_checksum=Read4Bytes(p_token).mChecksum;
	p_token+=4;
	p_token=SkipEndOfLines(p_token);
	Dbg_MsgAssert(*p_token==ESCRIPTTOKEN_EQUALS,("\nExpected an equals sign to follow '%s' at line %d, %s\nThe most likely explanation is that you have a command\noutside of a script ... endscript",FindChecksumName(name_checksum),GetLineNumber(p_token),p_fileName));
	++p_token;
	p_token=SkipEndOfLines(p_token);

	// We have a name followed by an equals, so this is defining some symbol with that name,
	// eg Foo=6, or Foo="Blaa"
	// So first of all, see if any symbol already exists with that name, and if it does then
	// remove it, or assert.
	CSymbolTableEntry *p_existing_entry=LookUpSymbol(name_checksum);
	if (p_existing_entry)
	{
		if (assertIfDuplicateSymbols)
		{
			Dbg_MsgAssert(0,("The symbol '%s' is defined twice, in %s and %s\n Try running CleanAss ",FindChecksumName(name_checksum),FindChecksumName(p_existing_entry->mSourceFileNameChecksum),p_fileName));
		}
		
		CleanUpAndRemoveSymbol(p_existing_entry);
	}		

	// Create a new symbol with the given name.
	CSymbolTableEntry *p_new=CreateNewSymbolEntry(name_checksum);
	Dbg_MsgAssert(p_new,("NULL p_new ??"));

	// Store the name of the source qb in the symbol so that the qb is able to be unloaded.
	// Note: This used to be after the switch statement below. Moved it here so that if any
	// assert goes off in the switch statement the file name info will be present in the symbol.
	p_new->mSourceFileNameChecksum=Crc::GenerateCRCFromString(p_fileName);

	// Now see what type of value follows the equals, and fill in the new symbol accordingly.
	switch (*p_token)
	{
		case ESCRIPTTOKEN_OPENPARENTH:
		{
			CComponent *p_comp=new CComponent;
			p_token=Evaluate(p_token,NULL,p_comp);
			Dbg_MsgAssert(p_comp->mType!=ESYMBOLTYPE_NONE,("Global symbol '%s' did not evaluate to anything ...",FindChecksumName(p_comp->mNameChecksum)));
			p_new->mType=p_comp->mType;
			p_new->mUnion=p_comp->mUnion;
			p_comp->mUnion=0;
			delete p_comp;
			break;
		}
		
		case ESCRIPTTOKEN_INTEGER:
		case ESCRIPTTOKEN_FLOAT:
		case ESCRIPTTOKEN_VECTOR:
		case ESCRIPTTOKEN_PAIR:
		case ESCRIPTTOKEN_STRING:
		case ESCRIPTTOKEN_LOCALSTRING:
		case ESCRIPTTOKEN_NAME:
		{
			CComponent *p_comp=new CComponent;
			p_token=FillInComponentUsingQB(p_token,NULL,p_comp);
			p_new->mType=p_comp->mType;
			p_new->mUnion=p_comp->mUnion;
			p_comp->mUnion=0;
			delete p_comp;
			break;
		}

		case ESCRIPTTOKEN_STARTSTRUCT:
		{
			p_new->mType=ESYMBOLTYPE_STRUCTURE;
			p_new->mpStructure=new CStruct;
			
			p_token=sAddComponentsWithinCurlyBraces(p_new->mpStructure,p_token);
			break;
		}
		case ESCRIPTTOKEN_STARTARRAY:
		{
			p_new->mType=ESYMBOLTYPE_ARRAY;
			
			p_new->mpArray=new CArray;
			p_token=sInitArrayFromQB(p_new->mpArray,p_token);
			break;                
		}	
			
		default:
			Dbg_MsgAssert(0,("\nUnexpected type value of %d given to symbol '%s', line %d of %s",*p_token,FindChecksumName(name_checksum),GetLineNumber(p_token),p_fileName));
			break;
	} // switch (*p_token)
	
	return p_token;
}

// Given a pointer to an un-preprocessed script, this will parse through it converting function names
// to either cfunction pointers or member function tokens.
void PreProcessScript(uint8 *p_token)
{
	// Skip over the default params
	p_token=SkipToStartOfNextLine(p_token);
	
	while (*p_token!=ESCRIPTTOKEN_KEYWORD_ENDSCRIPT) 
	{
		switch (*p_token)
		{
			case ESCRIPTTOKEN_KEYWORD_IF:
				++p_token;
				break;
				
			case ESCRIPTTOKEN_NAME:
			{
				++p_token;
				uint32 name_checksum=Read4Bytes(p_token).mChecksum;
				p_token+=4;
	
				// Look up the name to see if it is a cfunction or member function.
				CSymbolTableEntry *p_entry=Resolve(name_checksum);
				// Must not assert if p_entry is NULL, cos they might just be loading in
				// a qb file that refers to a script that has not been written yet.
				if (p_entry)
				{
					if (p_entry->mType==ESYMBOLTYPE_CFUNCTION)
					{
						// Change the token type and replace the checksum with the
						// function pointer to save having to resolve it later.
						*(p_token-5)=ESCRIPTTOKEN_RUNTIME_CFUNCTION;
						Dbg_MsgAssert(p_entry->mpCFunction,("NULL p_entry->mpCFunction"));
						Write4Bytes(p_token-4, (uint32)p_entry->mpCFunction);
					}
					else if (p_entry->mType==ESYMBOLTYPE_MEMBERFUNCTION)
					{
						// Saves having to look up the checksum later to find out that it is
						// a member function.
						*(p_token-5)=ESCRIPTTOKEN_RUNTIME_MEMBERFUNCTION;
					}	
				}		
				p_token=SkipToStartOfNextLine(p_token);
				break;
			}	
			default:
				p_token=SkipToStartOfNextLine(p_token);
				break;
		}
	}	
}

static CStoredRandom *sFindStoredRandom(const uint8 *p_token, EScriptToken type, int numItems)
{
	CScript *p_current_script=GetCurrentScript();
	
	CStoredRandom *p_search=sp_first_stored_random;
	while (p_search)
	{
		if (p_search->mpToken==p_token &&
			p_search->mType==type &&
			p_search->mNumItems==numItems)
		{
			if (p_search->mpScript==p_current_script || p_search->mpScript==NULL)
			{
				return p_search;
			}
		}
			
		p_search=p_search->mpNext;
	}	
	return NULL;
}

static CStoredRandom *sCreateNewStoredRandom()
{
	if (s_num_stored_randoms>=MAX_STORED_RANDOMS)
	{
		Dbg_MsgAssert(sp_last_stored_random,("NULL sp_last_stored_random ?"));
		
		CStoredRandom *p_new_last=sp_last_stored_random->mpPrevious;
		delete sp_last_stored_random;
		
		sp_last_stored_random=p_new_last;
		if (p_new_last)
		{
			p_new_last->mpNext=NULL;
		}	
	}
	
	CStoredRandom *p_new=new CStoredRandom;
	p_new->mpNext=sp_first_stored_random;
	p_new->mpPrevious=NULL;
	
	if (sp_first_stored_random)
	{
		sp_first_stored_random->mpPrevious=p_new;
	}	
	else
	{
		Dbg_MsgAssert(sp_last_stored_random==NULL,("sp_last_stored_random not NULL?"));
		sp_last_stored_random=p_new;
	}
	
	sp_first_stored_random=p_new;
	return p_new;
}

S4Bytes Read4Bytes(const uint8 *p_long)
{
    S4Bytes four_bytes;
	
	#ifdef __PLAT_NGPS__
	if ( (((uint32)p_long)&3)==0 )
	{
		// Small speed opt
		four_bytes.mUInt=*(uint32*)p_long;
	}
	else
	{
		four_bytes.mUInt=(p_long[0])|(p_long[1]<<8)|(p_long[2]<<16)|(p_long[3]<<24);
	}	
	#else
	four_bytes.mUInt=(p_long[0])|(p_long[1]<<8)|(p_long[2]<<16)|(p_long[3]<<24);
	#endif
    return four_bytes;
}

// Gets 2 bytes from p_short, which may not be long word aligned.
S2Bytes Read2Bytes(const uint8 *p_short)
{
    S2Bytes two_bytes;
    two_bytes.mUInt=p_short[0]|(p_short[1]<<8);
    return two_bytes;
}

// Used by WriteToBuffer and ReadFromBuffer in utils.cpp
// Moved to parse.cpp just so that all these byte reading/writing functions are in one place.
uint8 *Write4Bytes(uint8 *p_buffer, uint32 val)
{
	*p_buffer++=val;
	*p_buffer++=val>>8;
	*p_buffer++=val>>16;
	*p_buffer++=val>>24;
	return p_buffer;
}

uint8 *Write4Bytes(uint8 *p_buffer, float floatVal)
{
	uint32 val=*(uint32*)&floatVal;
	*p_buffer++=val;
	*p_buffer++=val>>8;
	*p_buffer++=val>>16;
	*p_buffer++=val>>24;
	return p_buffer;
}

uint8 *Write2Bytes(uint8 *p_buffer, uint16 val)
{
	*p_buffer++=val;
	*p_buffer++=val>>8;
	return p_buffer;
}

// Skips over end-of-line tokens.    
uint8 *SkipEndOfLines(uint8 *p_token)
{
	Dbg_MsgAssert(p_token,("NULL p_token"));
    while (true)
    {
		if (*p_token==ESCRIPTTOKEN_ENDOFLINE)
		{
            ++p_token;
		}	
        else if (*p_token==ESCRIPTTOKEN_ENDOFLINENUMBER)
		{
            // ESCRIPTTOKEN_ENDOFLINENUMBER contains the line number of the previous line,
            // so that the line number of errors can be displayed.
            p_token+=5;
		}	
        else   
		{
            break;
		}	
    }
    return p_token;
}

CStoredRandom::CStoredRandom()
{
	mpNext=NULL;
	mpPrevious=NULL;
	
	++s_num_stored_randoms;
}

CStoredRandom::~CStoredRandom()
{
	--s_num_stored_randoms;
}

// This gets called whenever CScript::ClearScript is called.
void ReleaseStoredRandoms(CScript *p_script)
{
	CStoredRandom *p_search=sp_first_stored_random;
	while (p_search)
	{
		if (p_search->mpScript==p_script)
		{
			p_search->mpScript=NULL;
		}
		p_search=p_search->mpNext;
	}	
}

void CStoredRandom::InitIndices()
{
	Dbg_MsgAssert(mNumItems<=MAX_RANDOM_INDICES,("mNumItems too big"));
	
	for (uint16 i=0; i<mNumItems; ++i)
	{
		mpIndices[i]=i;
	}	
}

void CStoredRandom::RandomizeIndices(bool makeNewFirstDifferFromOldLast)
{
	Dbg_MsgAssert(mNumItems>=2,("mNumItems must be at least 2 for RandomNoRepeat"));
	uint8 last=mpIndices[mNumItems-1];
	
	uint32 num_iterations=10*mNumItems;
	for (uint32 n=0; n<num_iterations; ++n)
	{
		int a=Mth::Rnd(mNumItems);
		int b=Mth::Rnd(mNumItems);
		uint8 temp=mpIndices[a];
		mpIndices[a]=mpIndices[b];
		mpIndices[b]=temp;
	}
	
	if (makeNewFirstDifferFromOldLast && mpIndices[0]==last)
	{
		int a=0;
		int b=1+Mth::Rnd(mNumItems-1);
		uint8 temp=mpIndices[a];
		mpIndices[a]=mpIndices[b];
		mpIndices[b]=temp;
	}	
}

// If p_token points to a random or jump token, then this will execute it,
// and repeat until no more randoms or jumps.
//
// Returns the new p_token, which is all this function modifies.
uint8 *DoAnyRandomsOrJumps(uint8 *p_token)
{
	Dbg_MsgAssert(p_token,("NULL p_token"));
	
	while (true)
	{
		bool finished=false;

		uint8 token=*p_token;
		switch (token)
		{
		case ESCRIPTTOKEN_KEYWORD_RANDOM:
		case ESCRIPTTOKEN_KEYWORD_RANDOM2:
		{
			++p_token; // Skip over the token.
			uint32 num_items=Read4Bytes(p_token).mUInt;
			Dbg_MsgAssert(num_items,("Zero num_items in random operator ?"));
			p_token+=4; 							// Skip over the num_items
			
			// Calculate the sum of the weights
			int sum_of_weights=0;
			uint8 *p_weights=p_token;
			for (uint32 i=0; i<num_items; ++i)
			{
				sum_of_weights+=Read2Bytes(p_weights).mInt;
				p_weights+=2;
			}	
			Dbg_MsgAssert(sum_of_weights<=32767,("sum_of_weights too big"));
			int r;
			if (token==ESCRIPTTOKEN_KEYWORD_RANDOM)
			{
				r=Mth::Rnd(sum_of_weights);
			}
			else
			{			 
				r=Mth::Rnd2(sum_of_weights);
			}	
			uint32 chosen_index=0;
			p_weights=p_token;
			while (true)
			{
				r-=Read2Bytes(p_weights).mInt;
				if (r<0)
				{
					break;
				}	
				p_weights+=2;
				++chosen_index;
				Dbg_MsgAssert(chosen_index<num_items,("chosen_index >= num_items ???"));
			}
			
			p_token+=2*num_items;					// Skip over the weights
			p_token+=4*chosen_index;				// Jump to the chosen offset value
			sint32 offset=Read4Bytes(p_token).mInt;	// Get the offset
			p_token+=4; 							// Skip over the offset
			p_token+=offset;						// Jump to the item
			break;
		}
		case ESCRIPTTOKEN_KEYWORD_RANDOM_NO_REPEAT:
		{
			++p_token; // Skip over the token.
			uint32 num_items=Read4Bytes(p_token).mUInt;
			Dbg_MsgAssert(num_items>1,("num_items must be greater than 1 in RandomNoRepeat operator"));
			p_token+=4; // Skip over the num_items
			p_token+=2*num_items;					// TODO: Skip over weights
			
			CStoredRandom *p_stored=sFindStoredRandom(p_token,ESCRIPTTOKEN_KEYWORD_RANDOM_NO_REPEAT,num_items);
			if (!p_stored)
			{
				p_stored=sCreateNewStoredRandom();
				Dbg_MsgAssert(p_stored,("NULL return value from sCreateNewStoredRandom()"));
				p_stored->mpToken=p_token;
				p_stored->mType=ESCRIPTTOKEN_KEYWORD_RANDOM_NO_REPEAT;
				p_stored->mNumItems=num_items;
				
				p_stored->mLastIndex=Mth::Rnd(num_items);
			}
			p_stored->mpScript=GetCurrentScript();
			
			uint16 jump_index=Mth::Rnd(num_items);
			// num_items is bigger than one, so in theory it should never get stuck in
			// an infinite loop here ...
			int c=0; // But have a counter just in case 
			while (jump_index==p_stored->mLastIndex)
			{
				++c;
				if (c>100) break;
				jump_index=Mth::Rnd(num_items);
			}	
			p_stored->mLastIndex=jump_index;
			
			p_token+=4*jump_index;	// Jump to the offset
			sint32 offset=Read4Bytes(p_token).mInt;	// Get the offset
			p_token+=4; 							// Skip over the offset
			p_token+=offset;						// Jump to the item
			break;
		}
		case ESCRIPTTOKEN_KEYWORD_RANDOM_PERMUTE:
		{
			++p_token; // Skip over the token.
			uint32 num_items=Read4Bytes(p_token).mUInt;
			Dbg_MsgAssert(num_items,("Zero num_items in random operator ?"));
			p_token+=4; // Skip over the num_items
			p_token+=2*num_items;					// TODO: Skip over weights
			
			CStoredRandom *p_stored=sFindStoredRandom(p_token,ESCRIPTTOKEN_KEYWORD_RANDOM_PERMUTE,num_items);
			if (!p_stored)
			{
				p_stored=sCreateNewStoredRandom();
				Dbg_MsgAssert(p_stored,("NULL return value from sCreateNewStoredRandom()"));
				p_stored->mpToken=p_token;
				p_stored->mType=ESCRIPTTOKEN_KEYWORD_RANDOM_PERMUTE;
				p_stored->mNumItems=num_items;

				p_stored->mCurrentIndex=0;
				Dbg_MsgAssert(num_items<=MAX_RANDOM_INDICES,("Too many entries in RandomPermute, max is %d. Line %d of file %s",MAX_RANDOM_INDICES,GetLineNumber(p_token),GetSourceFile(p_token)));
			
				p_stored->InitIndices();
				p_stored->RandomizeIndices();
			}
			p_stored->mpScript=GetCurrentScript();
			
			++p_stored->mCurrentIndex;
			if (p_stored->mCurrentIndex>=p_stored->mNumItems)
			{
				p_stored->RandomizeIndices(MAKE_NEW_FIRST_DIFFER_FROM_OLD_LAST);
				p_stored->mCurrentIndex=0;
			}
			uint16 jump_index=p_stored->mpIndices[p_stored->mCurrentIndex];
			Dbg_MsgAssert(jump_index<num_items,("Eh? jump_index out of range ?"));
			
			p_token+=4*jump_index;	// Jump to the offset
			sint32 offset=Read4Bytes(p_token).mInt;	// Get the offset
			p_token+=4; 							// Skip over the offset
			p_token+=offset;						// Jump to the item
			break;
		}
		case ESCRIPTTOKEN_JUMP:
		{
			++p_token; // Skip over the jump token.
            sint32 offset=Read4Bytes(p_token).mInt;
			p_token+=4; // Skip over the offset
			p_token+=offset; // Do the jump
			break;
		}
		default:
			finished=true;
			break;
		}
		
		if (finished)
		{
			break;
		}	
	}
	return p_token;
}		

// Returns the pointer to after the script.
uint8 *SkipOverScript(uint8 *p_token)
{
	Dbg_MsgAssert(p_token,("NULL p_token"));
	
	while (*p_token!=ESCRIPTTOKEN_KEYWORD_ENDSCRIPT) 
	{
		p_token=SkipToken(p_token);
	}	
	p_token=SkipToken(p_token); // Skip over the ESCRIPTTOKEN_KEYWORD_ENDSCRIPT
	
	// Include any line number info following the endscript. This is for use by the script
	// debugger, so that GetLineNumber will still work if the passed pointer points to an
	// endscript token.
	if (*p_token==ESCRIPTTOKEN_ENDOFLINENUMBER)
	{
		p_token=SkipToken(p_token);
	}
		
	return p_token;
}

// Calculates the checksum of a script, but ignores end-of-line tokens.
// This is used when reloading a script to detect whether the script has changed. If a script has
// changed, any instances of that script that are running will get restarted.
// Want to ignore end-of-lines and end-of-line-number tokens so that inserting or
// deleting lines won't cause all the scripts after that line to restart.
uint32 CalculateScriptContentsChecksum(uint8 *p_token)
{
	Dbg_MsgAssert(p_token,("NULL p_token"));

	uint32 checksum=0xffffffff;
		
	while (*p_token != ESCRIPTTOKEN_KEYWORD_ENDSCRIPT) 
	{
		uint8 *p_last_token=p_token;
		p_token=SkipToken(p_token);

		// Only include non end-of-line tokens in the checksum calculation.		
		if (*p_last_token != ESCRIPTTOKEN_ENDOFLINE &&
			*p_last_token != ESCRIPTTOKEN_ENDOFLINENUMBER)
		{
			int num_bytes=p_token-p_last_token;
			checksum=Crc::UpdateCRC((const char*)p_last_token,num_bytes,checksum);
		}
	}	
	
	return checksum;
}

// Given a token pointer, this will return the line number in the source q file.
// If it can't find a line number it returns 0. (Valid line numbers start at 1)
// If it gets to the end of the file it returns -1.
// If it gets to the end of a script it returns -2.
//
// Note: Perhaps make this compile to nothing for X-Box, since it is often used in asserts and I think
// the X-box will always execute functions in asserts, making parsing very slow. (Check with Dave)
int GetLineNumber(uint8 *p_token)
{
	if (!p_token)
	{
		return 0;
	}	
	
    switch (*p_token)
	{
	    case ESCRIPTTOKEN_ENDOFFILE:
	    case ESCRIPTTOKEN_ENDOFLINE:
	    case ESCRIPTTOKEN_ENDOFLINENUMBER:
	    case ESCRIPTTOKEN_STARTSTRUCT:
	    case ESCRIPTTOKEN_ENDSTRUCT:
	    case ESCRIPTTOKEN_STARTARRAY:
	    case ESCRIPTTOKEN_ENDARRAY:
	    case ESCRIPTTOKEN_EQUALS:
	    case ESCRIPTTOKEN_DOT:
	    case ESCRIPTTOKEN_COMMA:
	    case ESCRIPTTOKEN_MINUS:
	    case ESCRIPTTOKEN_ADD:
	    case ESCRIPTTOKEN_DIVIDE:
	    case ESCRIPTTOKEN_MULTIPLY:
	    case ESCRIPTTOKEN_OPENPARENTH:
	    case ESCRIPTTOKEN_CLOSEPARENTH:
	    case ESCRIPTTOKEN_DEBUGINFO:
	    case ESCRIPTTOKEN_SAMEAS:
	    case ESCRIPTTOKEN_LESSTHAN:
	    case ESCRIPTTOKEN_LESSTHANEQUAL:
	    case ESCRIPTTOKEN_GREATERTHAN:
	    case ESCRIPTTOKEN_GREATERTHANEQUAL:
	    case ESCRIPTTOKEN_NAME:
	    case ESCRIPTTOKEN_INTEGER:
	    case ESCRIPTTOKEN_HEXINTEGER:
        case ESCRIPTTOKEN_ENUM:
	    case ESCRIPTTOKEN_FLOAT:
	    case ESCRIPTTOKEN_STRING:
	    case ESCRIPTTOKEN_LOCALSTRING:
	    case ESCRIPTTOKEN_ARRAY:
	    case ESCRIPTTOKEN_VECTOR:
	    case ESCRIPTTOKEN_PAIR:
	    case ESCRIPTTOKEN_KEYWORD_BEGIN:
	    case ESCRIPTTOKEN_KEYWORD_REPEAT:
	    case ESCRIPTTOKEN_KEYWORD_BREAK:
	    case ESCRIPTTOKEN_KEYWORD_SCRIPT:
	    case ESCRIPTTOKEN_KEYWORD_ENDSCRIPT:
	    case ESCRIPTTOKEN_KEYWORD_IF:
	    case ESCRIPTTOKEN_KEYWORD_ELSE:
	    case ESCRIPTTOKEN_KEYWORD_ELSEIF:
	    case ESCRIPTTOKEN_KEYWORD_ENDIF:
	    case ESCRIPTTOKEN_KEYWORD_RETURN:
	    case ESCRIPTTOKEN_KEYWORD_NOT:
		case ESCRIPTTOKEN_KEYWORD_SWITCH:
		case ESCRIPTTOKEN_KEYWORD_ENDSWITCH:
		case ESCRIPTTOKEN_KEYWORD_CASE:
		case ESCRIPTTOKEN_KEYWORD_DEFAULT:
        case ESCRIPTTOKEN_UNDEFINED:
	    case ESCRIPTTOKEN_CHECKSUM_NAME:
	    case ESCRIPTTOKEN_KEYWORD_ALLARGS:
	    case ESCRIPTTOKEN_ARG:	
		case ESCRIPTTOKEN_JUMP:
		case ESCRIPTTOKEN_KEYWORD_RANDOM:
		case ESCRIPTTOKEN_KEYWORD_RANDOM2:
		case ESCRIPTTOKEN_KEYWORD_RANDOM_NO_REPEAT:
		case ESCRIPTTOKEN_KEYWORD_RANDOM_PERMUTE:
		case ESCRIPTTOKEN_OR:
		case ESCRIPTTOKEN_AND:
		case ESCRIPTTOKEN_XOR:
		case ESCRIPTTOKEN_SHIFT_LEFT:
		case ESCRIPTTOKEN_SHIFT_RIGHT:
		case ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE:
		case ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE2:
		case ESCRIPTTOKEN_COLON:
		case ESCRIPTTOKEN_RUNTIME_CFUNCTION:
		case ESCRIPTTOKEN_RUNTIME_MEMBERFUNCTION:
			break;
		default:
			Dbg_MsgAssert(0,("p_token does not point to a token in call to GetLineNumber"));
			break;
	}		
	
    while (true)
    {
        switch (*p_token)
		{
		case ESCRIPTTOKEN_ENDOFLINE:
			return 0;
			break;
			
		case ESCRIPTTOKEN_ENDOFLINENUMBER:
			++p_token;
			return Read4Bytes(p_token).mInt;
            break;
		
		case ESCRIPTTOKEN_ENDOFFILE:
			return -1;
			break;
			
		case ESCRIPTTOKEN_KEYWORD_ENDSCRIPT:
			p_token=SkipToken(p_token);
			if (*p_token==ESCRIPTTOKEN_ENDOFLINENUMBER)
			{
				// Continue so that the ENDOFLINENUMBER case catches this next time round the loop.
			}
			else
			{
				// Cannot continue searching because we've gone off the end
				// of the memory block allocated for the script.
				return -2;
			}	
			break;
			
		default:
			p_token=SkipToken(p_token);
			break;		
		}	
    }
}

// Returns true if the passed p_token points somewhere in the passed p_script.
bool PointsIntoScript(uint8 *p_token, uint8 *p_script)
{
	Dbg_MsgAssert(p_token,("NULL p_token"));
	Dbg_MsgAssert(p_script,("NULL p_script"));
	
	// Scan through the whole script to see if p_token points to
	// a token in it. Blimey.
	while (*p_script!=ESCRIPTTOKEN_KEYWORD_ENDSCRIPT)
	{
		if (p_script==p_token)
		{
			return true;
		}
		p_script=SkipToken(p_script);
	}		
	
	return false;
}

// Given a pointer to a token, this will try and find out what source file this is in.
const char *GetSourceFile(uint8 *p_token)
{
	// It may be that GetSourceFile was called due to an error when creating a script global.
	// In this case the source file cannot be determined from p_token since the qb is loaded into
	// a temporary buffer, but ParseQB will have set s_qb_being_parsed.
	if (s_qb_being_parsed)
	{
		return FindChecksumName(s_qb_being_parsed);
	}

	#ifdef NO_SCRIPT_CACHING
	// If there is no script caching, then the symbol table entries for each script contain pointers
	// to the uncompressed scripts, so it is possible to scan through them checking if p_token points
	// into one of them.
	
	// Check if pToken points into any of the scripts currently loaded.
	CSymbolTableEntry *p_sym=GetNextSymbolTableEntry(NULL);
	while (p_sym)
	{
		// For every script ...
		if (p_sym->mType==ESYMBOLTYPE_QSCRIPT)
		{
			uint8 *p_script=p_sym->mpScript;
			Dbg_MsgAssert(p_script,("NULL p_sym->mpScript ?"));
			// Skip over the contents checksum.
			p_script+=4;
			
			if (PointsIntoScript(p_token,p_script))
			{
				return FindChecksumName(p_sym->mSourceFileNameChecksum);
			}		
		}
		p_sym=GetNextSymbolTableEntry(p_sym);
	}
	#else
	
	// Script caching is on, so query the script cache to find out what the script name is.
	// Cannot scan through the symbols mpScript's as above because when script caching is
	// on mpScript will point to compressed data, which cannot be stepped through.
	// So ask the script cache to step through its decompressed scripts instead.
	Script::CScriptCache *p_script_cache=Script::CScriptCache::Instance();
	Dbg_MsgAssert(p_script_cache,("NULL p_script_cache"));
	return p_script_cache->GetSourceFile(p_token);
	
	#endif
	
	// Oh well.
	return "Unknown";	
}

#if 0
void CalcSpaceUsedByLineNumberInfo()
{
	int space_used=0;
	
	CSymbolTableEntry *p_sym=GetNextSymbolTableEntry(NULL);
	while (p_sym)
	{
		// For every script ...
		if (p_sym->mType==ESYMBOLTYPE_QSCRIPT)
		{
			uint8 *p_script=p_sym->mpScript;
			Dbg_MsgAssert(p_script,("NULL p_sym->mpScript ?"));
			// Skip over the contents checksum.
			p_script+=4;
			
			// Scan through the whole script looking for line number tokens.
			while (*p_script!=ESCRIPTTOKEN_KEYWORD_ENDSCRIPT)
			{
				if (*p_script==ESCRIPTTOKEN_ENDOFLINENUMBER)
				{
					space_used+=4;
				}	
				p_script=SkipToken(p_script);
			}		
		}
		p_sym=GetNextSymbolTableEntry(p_sym);
	}
	printf("Script heap space used by line number info = %d bytes\n");
}
#endif

uint8 *FillInComponentUsingQB(uint8 *p_token, CStruct *p_args, CComponent *p_comp)
{
	Dbg_MsgAssert(p_token,("NULL p_token"));
	Dbg_MsgAssert(p_comp,("NULL p_comp"));
	bool use_rnd2=false;
	
	switch (*p_token++)
	{
		case ESCRIPTTOKEN_INTEGER:
			p_comp->mType=ESYMBOLTYPE_INTEGER;
			p_comp->mIntegerValue=Read4Bytes(p_token).mInt;
			p_token+=4;
			break;
		case ESCRIPTTOKEN_FLOAT:
			p_comp->mType=ESYMBOLTYPE_FLOAT;
			p_comp->mFloatValue=Read4Bytes(p_token).mFloat;
			p_token+=4;
			break;
		case ESCRIPTTOKEN_NAME:
			p_comp->mType=ESYMBOLTYPE_NAME;
			p_comp->mChecksum=Read4Bytes(p_token).mChecksum;
			p_token+=4;
			break;
		case ESCRIPTTOKEN_STRING:
		{
			p_comp->mType=ESYMBOLTYPE_STRING;
			int len=Read4Bytes(p_token).mInt;
			p_token+=4;
			p_comp->mpString=CreateString((const char *)p_token);
			p_token+=len;
			break;
		}
		case ESCRIPTTOKEN_LOCALSTRING:
		{
			p_comp->mType=ESYMBOLTYPE_LOCALSTRING;
			int len=Read4Bytes(p_token).mInt;
			p_token+=4;
			p_comp->mpString=CreateString((const char *)p_token);
			p_token+=len;
			break;
		}
		case ESCRIPTTOKEN_VECTOR:
		{
			p_comp->mType=ESYMBOLTYPE_VECTOR;
			float x=Read4Bytes(p_token).mFloat;
			p_token+=4;
			float y=Read4Bytes(p_token).mFloat;
			p_token+=4;
			float z=Read4Bytes(p_token).mFloat;
			p_token+=4;
			p_comp->mpVector=new CVector;
			p_comp->mpVector->mX=x;
			p_comp->mpVector->mY=y;
			p_comp->mpVector->mZ=z;
			break;
		}	
		case ESCRIPTTOKEN_PAIR:
		{
			p_comp->mType=ESYMBOLTYPE_PAIR;
			float x=Read4Bytes(p_token).mFloat;
			p_token+=4;
			float y=Read4Bytes(p_token).mFloat;
			p_token+=4;
			p_comp->mpPair=new CPair;
			p_comp->mpPair->mX=x;
			p_comp->mpPair->mY=y;
			break;
		}	
		case ESCRIPTTOKEN_STARTSTRUCT:
		{
			// Rewind p_token to point to ESCRIPTTOKEN_STARTSTRUCT, because the 
			// sAddComponentsWithinCurlyBraces call requires it.
			--p_token;
			
			// No need to set which heap we're using, cos CStruct's and their CComponent's come off
			// their own pools.
			CStruct *p_structure=new CStruct;
			p_token=sAddComponentsWithinCurlyBraces(p_structure,p_token,p_args);
			
			p_comp->mType=ESYMBOLTYPE_STRUCTURE;
			p_comp->mpStructure=p_structure;
			break;
		}    
		case ESCRIPTTOKEN_STARTARRAY:
		{
			// Rewind p_token to point to ESCRIPTTOKEN_STARTARRAY, because the CArray::Parse call requires it.
			--p_token;
			
			// The CArray constructor is not hard-wired to use the script heap for it's buffer, so need
			// to make sure we're using the script heap here.
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
			CArray *p_array=new CArray;
			Mem::Manager::sHandle().PopContext();			
			
			p_token=sInitArrayFromQB(p_array,p_token,p_args);
			
			p_comp->mType=ESYMBOLTYPE_ARRAY;
			p_comp->mpArray=p_array;
			break;
		}
		case ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE2:
			use_rnd2=true;
			// Intentional lack of a break here.
		case ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE:
		{
			p_token=sCalculateRandomRange(p_token,p_comp,use_rnd2);
			break;
		}    

		case ESCRIPTTOKEN_KEYWORD_SCRIPT:
		{
			Dbg_MsgAssert(*p_token==ESCRIPTTOKEN_NAME,("\nKeyword 'script' must be followed by a name, line %d (file name not known, sorry)",GetLineNumber(p_token)));
			++p_token; // Skip over the ESCRIPTTOKEN_NAME
			p_comp->mNameChecksum=Read4Bytes(p_token).mChecksum;
			p_token+=4; // Skip over the name checksum
			
			// Skip p_token over the script tokens, and calculate the size.
			const uint8 *p_script=p_token;
			p_token=SkipOverScript(p_token);
			uint32 size=p_token-p_script;
			
			p_comp->mType=ESYMBOLTYPE_QSCRIPT;
			
			// Allocate a buffer off the script heap
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
			Dbg_MsgAssert(size,("Zero script size"));
			uint8 *p_new_script=(uint8*)Mem::Malloc(size);
			Mem::Manager::sHandle().PopContext();
			
			// Copy the script into the new buffer.
			const uint8 *p_source=p_script;
			uint8 *p_dest=p_new_script;
			for (uint32 i=0; i<size; ++i)
			{
				*p_dest++=*p_source++;
			}	
			
			// Give the new buffer to the component.
			p_comp->mpScript=p_new_script;
			
 			break;
		}	

		case ESCRIPTTOKEN_ARG:
		{
			Dbg_MsgAssert(*p_token==ESCRIPTTOKEN_NAME,("Expected '<' token to be followed by a name, File %s, line %d",GetSourceFile(p_token),GetLineNumber(p_token)));
			++p_token;
			uint32 arg_checksum=Read4Bytes(p_token).mChecksum;
			p_token+=4;
			if (p_args)
			{
				// Look for a parameter named arg_checksum in p_args, recursing into any referenced structures
				CComponent *p_found=p_args->FindNamedComponentRecurse(arg_checksum);
				if (p_found)
				{
					// Note: There is no copy-constructor for CComponent, because that would cause a
					// cyclic dependency between component.cpp/h and struct.cpp/h.
					// Instead, use the CopyComponent function defined in struct.cpp
					CopyComponent(p_comp,p_found);
				}
			}
			break;	
		}
		
		case ESCRIPTTOKEN_KEYWORD_ALLARGS:
		{
			if (p_args)
			{
				CStruct *p_structure=new CStruct;
				*p_structure=*p_args;
				
				p_comp->mType=ESYMBOLTYPE_STRUCTURE;
				p_comp->mpStructure=p_structure;
			}
			break;
		}		
		
		default:
			break;
	}
	return p_token;	
}

// This will evaluate the expression pointed to by p_token.
// Any args referred to by the <,> symbols are looked for in p_args.

// Expressions are terminated by any token that does not make sense as part of the expression.
// If the expression gets terminated unexpectedly, eg if there are still open braces, 
// then it will assert.
static CExpressionEvaluator sExpressionEvaluator;

void EnableExpressionEvaluatorErrorChecking()
{
	sExpressionEvaluator.EnableErrorChecking();
}

void DisableExpressionEvaluatorErrorChecking()
{
	sExpressionEvaluator.DisableErrorChecking();
}
	
static CComponent sTemp;
uint8 *Evaluate(uint8 *p_token, CStruct *p_args, CComponent *p_result)
{
	Dbg_MsgAssert(p_result,("NULL p_result"));
	Dbg_MsgAssert(p_token,("NULL p_token"));
	
	// SPEEDOPT: Make the Clear function faster
	sExpressionEvaluator.ClearIfNeeded();
	sExpressionEvaluator.SetTokenPointer(p_token);
	
	Dbg_MsgAssert(*p_token==ESCRIPTTOKEN_OPENPARENTH,("Expected expression to begin with a '(', at line %d of %s",GetLineNumber(p_token),GetSourceFile(p_token)));
	++p_token;
	
	int parenth_count=0;
	bool generate_minus_operator=false;
	bool expecting_value=true;
	bool operator_is_dot=false;
	
	bool in_expression=true;
	while (in_expression)
	{
		p_token=DoAnyRandomsOrJumps(p_token);
		uint8 token_value=*p_token;
		
		switch (token_value)	
		{
		case ESCRIPTTOKEN_OPENPARENTH:
			if (!expecting_value)
			{
				in_expression=false;
				break;
			}	
			++p_token;
			sExpressionEvaluator.OpenParenthesis();
			generate_minus_operator=false;
			expecting_value=true;
			++parenth_count;
			break;
		case ESCRIPTTOKEN_CLOSEPARENTH:
			++p_token;
			if (parenth_count)
			{
				sExpressionEvaluator.CloseParenthesis();
				generate_minus_operator=true;
				expecting_value=false;
				--parenth_count;
			}
			else
			{
				in_expression=false;
			}	
			break;
		case ESCRIPTTOKEN_INTEGER:
			if (expecting_value)
			{
				Dbg_MsgAssert(!generate_minus_operator,("Eh? generate_minus_operator is set?"));
				p_token=FillInComponentUsingQB(p_token,p_args,&sTemp);
				
				sExpressionEvaluator.Input(&sTemp);
				CleanUpComponent(&sTemp);
				generate_minus_operator=true;
				expecting_value=false;
			}	
			else
			{
				uint8 *p_new_token=FillInComponentUsingQB(p_token,p_args,&sTemp);
				
				if (sTemp.mIntegerValue<0 && generate_minus_operator)
				{
					sExpressionEvaluator.Input(ESCRIPTTOKEN_MINUS);
					
					sTemp.mIntegerValue=-sTemp.mIntegerValue;
					sExpressionEvaluator.Input(&sTemp);
					CleanUpComponent(&sTemp);
					
					p_token=p_new_token;
					generate_minus_operator=true;
					expecting_value=false;
				}	
				else
				{
					in_expression=false;
				}	
			}	
			break;
			
		case ESCRIPTTOKEN_FLOAT:
			if (expecting_value)
			{
				Dbg_MsgAssert(!generate_minus_operator,("Eh? generate_minus_operator is set?"));
				p_token=FillInComponentUsingQB(p_token,p_args,&sTemp);
				
				sExpressionEvaluator.Input(&sTemp);
				CleanUpComponent(&sTemp);
				generate_minus_operator=true;
				expecting_value=false;
			}	
			else
			{
				uint8 *p_new_token=FillInComponentUsingQB(p_token,p_args,&sTemp);
				
				if (sTemp.mFloatValue<0.0f && generate_minus_operator)
				{
					sExpressionEvaluator.Input(ESCRIPTTOKEN_MINUS);
					
					sTemp.mFloatValue=-sTemp.mFloatValue;
					sExpressionEvaluator.Input(&sTemp);
					CleanUpComponent(&sTemp);
					
					p_token=p_new_token;
					generate_minus_operator=true;
					expecting_value=false;
				}	
				else
				{
					in_expression=false;
				}	
			}	
			break;
		case ESCRIPTTOKEN_NAME:
		{
			if (!expecting_value)
			{
				in_expression=false;
				break;
			}	
			p_token=FillInComponentUsingQB(p_token,p_args,&sTemp);
			
			// TODO: Use ResolveNameComponent instead of this switch ...
            CSymbolTableEntry *p_entry=NULL;
			if (operator_is_dot)
			{
				// This is a bit of a hack ...
				// If the operator is the dot operator, then do not check if the right-hand-side
				// is a global. This is because when the . is used to access members of a structure,
				// it may be that the name of the parameter being accessed is also the name of a 
				// global, which if substituted in place of the name would result in a nonsense 
				// expression.
				// The only disadvantage of this hack is that it means that a vector can no longer be
				// dot producted with a global vector, at least not directly.
				// I don't think there even are any global vectors at the moment anyway, and if there
				// were they could still be dot-producted with by simply copying them into a parameter 
				// and dotting with that instead.
				
				// The more general solution to the problem would be to have some sort of special
				// syntax to indicate when a name is not to be resolved. Eg, Foo.Blaa would mean 
				// that Blaa would be resolved, whereas Foo.~Blaa would tell the interpreter not to
				// resolve it.
				// That would be more complex to implement though.
			}
			else
			{
				p_entry=Resolve(sTemp.mChecksum);
			}
				
			if (p_entry)
			{
				switch (p_entry->mType)
				{
				case ESYMBOLTYPE_FLOAT:
					sTemp.mType=ESYMBOLTYPE_FLOAT;
					sTemp.mFloatValue=p_entry->mFloatValue;
					sExpressionEvaluator.Input(&sTemp);
					break;
				case ESYMBOLTYPE_INTEGER:
					sTemp.mType=ESYMBOLTYPE_INTEGER;
					sTemp.mIntegerValue=p_entry->mIntegerValue;
					sExpressionEvaluator.Input(&sTemp);
					break;
				case ESYMBOLTYPE_VECTOR:
					sTemp.mType=ESYMBOLTYPE_VECTOR;
					sTemp.mpVector=p_entry->mpVector;
					sExpressionEvaluator.Input(&sTemp);
					break;
				case ESYMBOLTYPE_PAIR:
					sTemp.mType=ESYMBOLTYPE_PAIR;
					sTemp.mpPair=p_entry->mpPair;
					sExpressionEvaluator.Input(&sTemp);
					break;
				case ESYMBOLTYPE_STRING:
					sTemp.mType=ESYMBOLTYPE_STRING;
					sTemp.mpString=p_entry->mpString;
					sExpressionEvaluator.Input(&sTemp);
					break;
				case ESYMBOLTYPE_LOCALSTRING:
					sTemp.mType=ESYMBOLTYPE_LOCALSTRING;
					sTemp.mpLocalString=p_entry->mpLocalString;
					sExpressionEvaluator.Input(&sTemp);
					break;
				case ESYMBOLTYPE_STRUCTURE:
					sTemp.mType=ESYMBOLTYPE_STRUCTURE;
					sTemp.mpStructure=p_entry->mpStructure;
					sExpressionEvaluator.Input(&sTemp);
					break;
				case ESYMBOLTYPE_ARRAY:
					sTemp.mType=ESYMBOLTYPE_ARRAY;
					sTemp.mpArray=p_entry->mpArray;
					sExpressionEvaluator.Input(&sTemp);
					break;
				case ESYMBOLTYPE_CFUNCTION:
				{
					CScript *p_script=GetCurrentScript();
					Dbg_MsgAssert(p_script,("Tried to execute the cfunc '%s' in an expression with no parent script, line %d of file %s",FindChecksumName(sTemp.mChecksum),GetLineNumber(p_token),GetSourceFile(p_token)));
					
					sTemp.mType=ESYMBOLTYPE_INTEGER;
				
					CStruct *p_function_params=new CStruct;
					// TODO: Problem if AddComponentsUntilEndOfLine calls Evaluate() again,
					// since only one instance of CExpressionEvaluator.
					p_token=AddComponentsUntilEndOfLine(p_function_params,p_token,p_args);
	                sTemp.mIntegerValue=(*p_entry->mpCFunction)(p_function_params,p_script);
					delete p_function_params;
					
					sExpressionEvaluator.Input(&sTemp);
					break;
				}	
				
				case ESYMBOLTYPE_MEMBERFUNCTION:
					Dbg_MsgAssert(0,("Sorry, cannot have calls to member functions in expressions!\nTried to call member function '%s' in an expression, line %d of file %s",FindChecksumName(sTemp.mChecksum),GetLineNumber(p_token),GetSourceFile(p_token)));
					break;
				
				default:
					sExpressionEvaluator.Input(&sTemp);
					break;
				}
			}
			else
			{
				sExpressionEvaluator.Input(&sTemp);
			}
			// Must not call CleanUpComponent because any pointer in the component will
			// have been borrowed from the global symbol.
			sTemp.mType=ESYMBOLTYPE_NONE;
			sTemp.mUnion=0;
			generate_minus_operator=true;
			expecting_value=false;
			break;
		}	
		case ESCRIPTTOKEN_STRING:
		case ESCRIPTTOKEN_LOCALSTRING:
		case ESCRIPTTOKEN_VECTOR:
		case ESCRIPTTOKEN_PAIR:
		//case ESCRIPTTOKEN_STARTARRAY:
		case ESCRIPTTOKEN_STARTSTRUCT:
		case ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE:
			if (!expecting_value)
			{
				in_expression=false;
				break;
			}	
			p_token=FillInComponentUsingQB(p_token,p_args,&sTemp);
			sExpressionEvaluator.Input(&sTemp);
			CleanUpComponent(&sTemp);
			generate_minus_operator=true;
			expecting_value=false;
			break;
		
		case ESCRIPTTOKEN_ARG:
		{
			if (!expecting_value)
			{
				in_expression=false;
				break;
			}	
			++p_token;
			Dbg_MsgAssert(*p_token==ESCRIPTTOKEN_NAME,("Expected '<' token to be followed by a name, File %s, line %d",GetSourceFile(p_token),GetLineNumber(p_token)));
			++p_token;
			uint32 arg_checksum=Read4Bytes(p_token).mChecksum;
			p_token+=4;
			if (p_args)
			{
				// Look for a parameter named arg_checksum in p_args, recursing into any referenced structures
				CComponent *p_comp=p_args->FindNamedComponentRecurse(arg_checksum);
				if (p_comp)
				{
					sTemp.mType=p_comp->mType;
					sTemp.mUnion=p_comp->mUnion;
					// It might be a name that resolves to some global, so resolve it.
					ResolveNameComponent(&sTemp);

					sExpressionEvaluator.Input(&sTemp);
					
					// Must not call CleanUpComponent because any pointer in the component will
					// have been borrowed from the global symbol or p_comp.
					sTemp.mType=ESYMBOLTYPE_NONE;
					sTemp.mUnion=0;
				}
			}	
			generate_minus_operator=true;
			expecting_value=false;
			break;	
		}
		case ESCRIPTTOKEN_KEYWORD_ALLARGS:
		{
			if (!expecting_value)
			{
				in_expression=false;
				break;
			}	
			++p_token;
			
			if (p_args)
			{
				sTemp.mType=ESYMBOLTYPE_STRUCTURE;
				sTemp.mpStructure=p_args;
				sExpressionEvaluator.Input(&sTemp);
			}	
			generate_minus_operator=true;
			expecting_value=false;
			break;	
		}
		
		case ESCRIPTTOKEN_ADD:
		case ESCRIPTTOKEN_MINUS:
		case ESCRIPTTOKEN_MULTIPLY:
		case ESCRIPTTOKEN_DIVIDE:
		case ESCRIPTTOKEN_DOT:
		case ESCRIPTTOKEN_OR:
		case ESCRIPTTOKEN_AND:
		case ESCRIPTTOKEN_LESSTHAN:
		case ESCRIPTTOKEN_GREATERTHAN:
		case ESCRIPTTOKEN_EQUALS:
		case ESCRIPTTOKEN_STARTARRAY:
			++p_token;
			sExpressionEvaluator.Input((EScriptToken)token_value);
			generate_minus_operator=false;
			expecting_value=true;
			if (token_value==ESCRIPTTOKEN_DOT)
			{
				operator_is_dot=true;
			}	
			else
			{
				operator_is_dot=false;
			}	
			break;
			
		case ESCRIPTTOKEN_ENDARRAY:
		case ESCRIPTTOKEN_ENDOFLINE:
			++p_token;
			break;
		case ESCRIPTTOKEN_ENDOFLINENUMBER:
			p_token+=5;
			break;
			
		case ESCRIPTTOKEN_ENDOFFILE:
		case ESCRIPTTOKEN_KEYWORD_ENDSCRIPT:
		case ESCRIPTTOKEN_COMMA:
			Dbg_MsgAssert(0,("Unexpected token '%s' in expression, line %d of %s",GetTokenName((EScriptToken)token_value),GetLineNumber(p_token),GetSourceFile(p_token)));
			break;	
			
		default:
			// Unrecognized expression
			Dbg_MsgAssert(0,("Don't know how to evaluate '%s', at line %d of %s",GetTokenName((EScriptToken)token_value),GetLineNumber(p_token),GetSourceFile(p_token)));
			break;	
		}	
		
		// This cannot be commented in all the time, because often the evaluator will report
		// errors when it gets called via the FindReferences function, which scans through scripts
		// but does not execute them. Errors occur in this case when script parameters are used
		// in expressions, because when FindReferences is scanning through the script it has no
		// parameters. Need to fix FindReferences so that it can scan over expressions without
		// executing them.
		//if (sExpressionEvaluator.GetError())
		//{
		//	Dbg_MsgAssert(0,("Evaluator error: File %s, line %d",GetSourceFile(p_token),GetLineNumber(p_token)));
		//}	
		#ifdef	__NOPT_ASSERT__
		if (sExpressionEvaluator.ErrorCheckingEnabled() && sExpressionEvaluator.GetError())
		{
			printf("Evaluator error: File %s, line %d",GetSourceFile(p_token),GetLineNumber(p_token));
		}	
		#endif
	}
		
	sExpressionEvaluator.GetResult(p_result);
	#ifdef	__NOPT_ASSERT__
	if (sExpressionEvaluator.ErrorCheckingEnabled() && sExpressionEvaluator.GetError())
	{
		printf("Evaluator error: File %s, line %d\n",GetSourceFile(p_token),GetLineNumber(p_token));
	}	
	#endif
	return p_token;
}

// Parses the components in QB format pointed to by p_token, and adds them to p_dest until the 
// end-of-line is reached.
// For example, p_token may point to: x=9 foo="Blaa"
// Used when creating the structure of function parameters, and the structure of default parameters
// for scripts.
// p_args contains the structure defining the value of any args referred to using the <,> operators.
// Returns a pointer to the next token after parsing the tokens pointed to by p_token.
//
// Note: This gets called for every line of script executed, hence is probably a bottleneck in script
// execution speed. Could speed up a lot by pre-generating any function parameters that are constant, ie
// contain no <,> operators.
uint8 *AddComponentsUntilEndOfLine(CStruct *p_dest, uint8 *p_token, CStruct *p_args)
{
	Dbg_MsgAssert(p_dest,("NULL p_dest"));
	Dbg_MsgAssert(p_token,("NULL p_token"));
	
    while (true)
    {
		// Execute any random operators.
		p_token=DoAnyRandomsOrJumps(p_token);

        if (sIsEndOfLine(p_token)) 
		{
			break;
		}	
		
		// This is so that function calls can be put in expressions, which are always enclosed
		// in parentheses.
		if (*p_token==ESCRIPTTOKEN_CLOSEPARENTH)
		{
			break;
		}
			
        switch (*p_token)
        {
            case ESCRIPTTOKEN_NAME:
            {
                uint8 *p_name_token=p_token;
                ++p_token;
                uint32 name_checksum=Read4Bytes(p_token).mChecksum;
                p_token+=4;
				// Execute any random operators. This has to be done each time p_token is advanced.
				p_token=DoAnyRandomsOrJumps(p_token);

                if (*p_token==ESCRIPTTOKEN_EQUALS)
                {
                    ++p_token;
					p_token=DoAnyRandomsOrJumps(p_token);
					
                    Dbg_MsgAssert(!sIsEndOfLine(p_token),("Syntax error, nothing following '=', File %s, line %d",GetSourceFile(p_token),GetLineNumber(p_token)));
					
					if (*p_token==ESCRIPTTOKEN_OPENPARENTH)
					{
						CComponent *p_comp=new CComponent;
						p_token=Evaluate(p_token,p_args,p_comp);
						if (p_comp->mType!=ESYMBOLTYPE_NONE)
						{
							p_comp->mNameChecksum=name_checksum;
							p_dest->AddComponent(p_comp);
						}
						else
						{
							delete p_comp;
						}
					}
					else
					{
						p_token=sAddComponentFromQB(p_dest,name_checksum,p_token,p_args);					
					}	
                }
                else
				{
                    p_token=sAddComponentFromQB(p_dest,NO_NAME,p_name_token,p_args);
				}	
                break;
            }
			case ESCRIPTTOKEN_KEYWORD_ALLARGS:
                ++p_token;
				if (p_args)
				{
					*p_dest+=*p_args;
				}	
				break;
            case ESCRIPTTOKEN_STARTSTRUCT:
                p_token=sAddComponentsWithinCurlyBraces(p_dest,p_token,p_args);
                break;
			case ESCRIPTTOKEN_ARG:
				p_token=sAddComponentFromQB(p_dest,NO_NAME,p_token,p_args);
				break;
            case ESCRIPTTOKEN_ENDOFLINE:
            case ESCRIPTTOKEN_ENDOFLINENUMBER:
                break;
            case ESCRIPTTOKEN_ENDOFFILE:
				Dbg_MsgAssert(0,("End of file reached during AddComponentsUntilEndOfLine ?"));
                break;
			
			case ESCRIPTTOKEN_COMMA:
				++p_token;
				break;
				
            default:
			{
				if (*p_token==ESCRIPTTOKEN_OPENPARENTH)
				{
					CComponent *p_comp=new CComponent;
					p_token=Evaluate(p_token,p_args,p_comp);
					
					if (p_comp->mType==ESYMBOLTYPE_STRUCTURE)
					{
						// The CStruct::AddComponent function does not allow unnamed structure
						// components to be added, so merge in the contents of the structure instead.
						p_dest->AppendStructure(p_comp->mpStructure);
						// Now p_comp does have to be cleaned up and deleted, because it has not
						// been given to p_dest.
						CleanUpComponent(p_comp);
						delete p_comp;
					}
					else
					{
						if (p_comp->mType!=ESYMBOLTYPE_NONE)
						{
							p_comp->mNameChecksum=NO_NAME;
							p_dest->AddComponent(p_comp);
						}
						else
						{
							delete p_comp;
						}	
					}				
				}
				else
				{
					p_token=sAddComponentFromQB(p_dest,NO_NAME,p_token,p_args);
				}	
                break;
			}	
        }            
    } 

	// Note: Does not skip over the end-of-lines. This is so that if any asserts go off during the
	// C-function or member function call that this structure is going to be used as parameters too,
	// the assert will print the correct line number, rather than the one following.
    return p_token;
}

// Deletes any entity pointed to by p_sym, then removes p_sym from the symbol table.
void CleanUpAndRemoveSymbol(CSymbolTableEntry *p_sym)
{
	Dbg_MsgAssert(p_sym,("NULL p_sym"));
	switch (p_sym->mType)
	{
		case ESYMBOLTYPE_INTEGER:
		case ESYMBOLTYPE_FLOAT:
		case ESYMBOLTYPE_NAME:
			// Nothing to delete
			break;		
			
		case ESYMBOLTYPE_STRING:
			Dbg_MsgAssert(p_sym->mpString,("NULL p_sym->mpString"));
			DeleteString(p_sym->mpString);
			break;
		case ESYMBOLTYPE_LOCALSTRING:
			Dbg_MsgAssert(p_sym->mpLocalString,("NULL p_sym->mpLocalString"));
			DeleteString(p_sym->mpLocalString);
			break;
		case ESYMBOLTYPE_PAIR:
			Dbg_MsgAssert(p_sym->mpPair,("NULL p_sym->mpPair"));
			delete p_sym->mpPair;
			break;
		case ESYMBOLTYPE_VECTOR:
			Dbg_MsgAssert(p_sym->mpVector,("NULL p_sym->mpVector"));
			delete p_sym->mpVector;
			break;
		case ESYMBOLTYPE_QSCRIPT:
#ifdef __PLAT_NGC__
			NsARAM::free( p_sym->mScriptOffset );
#else
			Dbg_MsgAssert(p_sym->mpScript,("NULL p_sym->mpScript"));
			Mem::Free(p_sym->mpScript);
#endif		// __PLAT_NGC__
			break;
		case ESYMBOLTYPE_STRUCTURE:
			Dbg_MsgAssert(p_sym->mpStructure,("NULL p_sym->mpStructure"));
			delete p_sym->mpStructure;
			break;
		case ESYMBOLTYPE_ARRAY:
			Dbg_MsgAssert(p_sym->mpArray,("NULL p_sym->mpArray"));
			CleanUpArray(p_sym->mpArray);
			delete p_sym->mpArray;
			break;
		default:
			Dbg_MsgAssert(0,("Bad type of '%s' in p_sym",GetTypeName(p_sym->mType)));
			break;
	}
	p_sym->mUnion=0;

	RemoveSymbol(p_sym);
}

// TODO: Remove? Currently used in cfuncs.cpp to delete the NodeArray, but now that we
// have the ability to unload qb files that should not be necessary any more.
// Note: Just found it is also needed in parkgen.cpp, so don't remove after all. 
void CleanUpAndRemoveSymbol(uint32 checksum)
{
	CSymbolTableEntry *p_sym=LookUpSymbol(checksum);
	if (p_sym)
	{
		CleanUpAndRemoveSymbol(p_sym);
	}	
}

void CleanUpAndRemoveSymbol(const char *p_symbolName)
{
	CleanUpAndRemoveSymbol(Crc::GenerateCRCFromString(p_symbolName));
}
	
#define HASHBITS 12
struct SChecksum
{
	uint32 mChecksum;
	char *mpName;
	SChecksum *mpNext;
};

static SChecksum *sp_hash_table=NULL;

const char *GetChecksumNameFromLastQB(uint32 checksum)
{
	Dbg_MsgAssert(sp_hash_table,("NULL sp_hash_table"));
	
	SChecksum *p_entry=&sp_hash_table[checksum&((1<<HASHBITS)-1)];
	while (p_entry)
	{
		if (p_entry->mChecksum==checksum)
		{
			return p_entry->mpName;
		}	
		p_entry=p_entry->mpNext;
	}	
	
	return NULL;
}	

void RemoveChecksumNameLookupHashTable()
{
	if (sp_hash_table)
	{
		SChecksum *p_entry=sp_hash_table;
		for (uint32 i=0; i<(1<<HASHBITS); ++i)
		{
			SChecksum *p_ch=p_entry->mpNext;
			while (p_ch)
			{
				SChecksum *p_next=p_ch->mpNext;
				// The dynamically allocated entries must have had their mpName set.
				Dbg_MsgAssert(p_ch->mpName,("NULL p_ch->mpName ?"));
				Mem::Free(p_ch->mpName);
				Mem::Free(p_ch);
				p_ch=p_next;
			}
			// The entries in the contiguous array may not have their mpName set.	
			if (p_entry->mpName)
			{
				Mem::Free(p_entry->mpName);
			}	
			++p_entry;
		}	
	
		Mem::Free(sp_hash_table);
		sp_hash_table=NULL;
	}	
}

uint8 *SkipToStartOfNextLine(uint8 *p_token)
{
	int curly_bracket_count=0;
	int square_bracket_count=0;
	int parenth_count=0;
	
	while (true)
	{
		if (*p_token==ESCRIPTTOKEN_STARTSTRUCT) ++curly_bracket_count;
		if (*p_token==ESCRIPTTOKEN_ENDSTRUCT) --curly_bracket_count;
		Dbg_MsgAssert(curly_bracket_count>=0,("Negative curly_bracket_count ??"));

		if (*p_token==ESCRIPTTOKEN_STARTARRAY) ++square_bracket_count;
		if (*p_token==ESCRIPTTOKEN_ENDARRAY) --square_bracket_count;
		Dbg_MsgAssert(square_bracket_count>=0,("Negative square_bracket_count ??"));

		if (*p_token==ESCRIPTTOKEN_OPENPARENTH) ++parenth_count;
		if (*p_token==ESCRIPTTOKEN_CLOSEPARENTH) --parenth_count;
		Dbg_MsgAssert(parenth_count>=0,("Negative parenth_count, File %s, line %d",GetSourceFile(p_token),GetLineNumber(p_token)));
		
		if (curly_bracket_count==0 && 
			square_bracket_count==0 &&
			parenth_count==0 &&
			sIsEndOfLine(p_token))
		{
			break;
		}		
		p_token=SkipToken(p_token);
	}
	
	p_token=SkipToken(p_token);
	return p_token;
}

// Parses a qb file, creating all the symbols (scripts, arrays etc) defined within.
// If the assertIfDuplicateSymbols is set, then it will assert if any of the symbols in the qb
// exist already.
// This flag is set when loading all the startup qb's, because we don't want clashing symbols.
// It is not set when reloading a qb during development though, since that involves reloading
// existing symbols.
// The file name is also passed so that each symbol knows what qb it came from, which allows
// all the symbols from a particular qb to be unloaded using the UnloadQB function (in file.cpp)
void ParseQB(const char *p_fileName, uint8 *p_qb, EBoolAssertIfDuplicateSymbols assertIfDuplicateSymbols, bool allocateChecksumNameLookupTable)
{
	Dbg_MsgAssert(p_fileName,("NULL p_fileName"));
	Dbg_MsgAssert(p_qb,("NULL p_qb"));

	// Do a first parse through the qb to register the checksum names.
	// They get added to a lookup table that can be queried using GetChecksumNameFromLastQB defined above.
	// The lookup table only contains the checksum names defined in this qb.
	// GetChecksumNameFromLastQB is currently only used by the game-specific nodearray.cpp to generate a lookup
	// table of node name prefixes.
	//
	// If __NOPT_ASSERT__ is set the checksum names also get added to a big permanent lookup table so that
	// the FindChecksumName function can be used at any time in asserts.
	// 
    uint8 *p_token=p_qb;
    bool end_of_file=false;

	// Remove any existing checksum-name-lookup hash table.
	RemoveChecksumNameLookupHashTable();

	// K: Added the passed flag allocateChecksumNameLookupTable so that we can avoid allocating the table
	// if we know it will not be needed. Gary needs this for the cutscenes where there is not enough memory
	// to allocate the table.
	// By default the flag is set to true however because the table is needed when generating prefix info
	// for the nodearray, and we don't know whether the qb contains the nodearray until after finishing
	// parsing it.
	if (allocateChecksumNameLookupTable)
	{
		// Reallocate it.
		bool use_debug_heap=false;
		#ifdef	__NOPT_ASSERT__	
		if (!assertIfDuplicateSymbols)
		{
			// They're doing a qbr ...
			if (Mem::Manager::sHandle().GetHeap(0x70cb0238/*DebugHeap*/))
			{
				// The debug heap exists, so use it.
				// Need to do this because there is often not enough memory otherwise.
				use_debug_heap=true;
			}	
		}	
		#endif
	
		if (use_debug_heap)
		{
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().DebugHeap());
		}	
		else
		{
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
		}
		Dbg_MsgAssert(sp_hash_table==NULL,("sp_hash_table not NULL ?"));
		sp_hash_table=(SChecksum*)Mem::Malloc((1<<HASHBITS)*sizeof(SChecksum));
		Dbg_MsgAssert(sp_hash_table,("NULL sp_hash_table"));
		Mem::Manager::sHandle().PopContext();
		
		// Initialise it.
		SChecksum *p_entry=sp_hash_table;
		for (uint32 i=0; i<(1<<HASHBITS); ++i)
		{
			p_entry->mChecksum=0;
			p_entry->mpName=NULL;
			p_entry->mpNext=NULL;
			++p_entry;
		}	
	}
	
	// Line number info in qb's is excluded when doing a release build to save memory.
	// Need to stop with an error message if there is line number info when there should not
	// be, otherwise the release build will run out of memory & it will be hard to track down
	// why since there will be no asserts.
	// This flag indicates whether to give the error message or not.
	bool allow_line_number_info=false;
	if (!assertIfDuplicateSymbols)
	{
		// Doing a qbr, so allow line number info, otherwise
		// we won't be able to qbr when __NOPT_ASSERT__ is not defined.
		allow_line_number_info=true;
	}	
	#ifdef __NOPT_ASSERT__
	// Allow line number info when asserts are on, cos the script heap will have an extra 200K for them.
	allow_line_number_info=true;
	#endif
	#ifdef __PLAT_WN32__
	// PC has lots of memory.
	allow_line_number_info=true;
	#endif
		

    while (!end_of_file)
	{
		switch (*p_token)
		{
		case ESCRIPTTOKEN_CHECKSUM_NAME: 
		{
			++p_token;
			uint32 checksum=Read4Bytes(p_token).mChecksum;
			const char *p_name=(const char*)(p_token+4);
	
			if (allocateChecksumNameLookupTable)
			{
				// Add it to the table.
				SChecksum *p_entry=&sp_hash_table[checksum&((1<<HASHBITS)-1)];
				if (p_entry->mpName)
				{
					// The first slot is already occupied, so create a new SChecksum and
					// link it in between the first and the rest.
					SChecksum *p_new=(SChecksum*)Mem::Malloc(sizeof(SChecksum));
					p_new->mChecksum=checksum;
					p_new->mpName=(char*)Mem::Malloc(strlen(p_name)+1);
					strcpy(p_new->mpName,p_name);
					
					p_new->mpNext=p_entry->mpNext;
					p_entry->mpNext=p_new;
				}
				else
				{
					// First slot is not occupied, so stick it in there.
					p_entry->mChecksum=checksum;
					p_entry->mpName=(char*)Mem::Malloc(strlen(p_name)+1);
					strcpy(p_entry->mpName,p_name);
					p_entry->mpNext=NULL;
				}	
			}
						
			// Add to the big lookup table of all checksum names, used by FindChecksumName
			// Doing this here rather than the main parsing loop after this one so that the names 
			// are registered first in case any asserts go off.
			AddChecksumName(checksum,p_name);
				
			// Skip over the checksum.
			p_token+=4;
			// Skip over the name.
			while (*p_token)
			{
				++p_token;
			}
			// Skip over the terminator.
			++p_token;	
			break;
		}
			
        case ESCRIPTTOKEN_ENDOFFILE:
            end_of_file=true;
            break;

		case ESCRIPTTOKEN_ENDOFLINENUMBER:
		{
			if (allow_line_number_info)
			{
				p_token=SkipToken(p_token);
			}
			else
			{
				printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
				printf("Found line number info in '%s' !\nLine number info needs to be removed when __NOPT_ASSERT__ is not defined\nin order to save memory.\nTry doing a 'cleanass nolinenumbers' and running again.\n",p_fileName);
				while (1);
			}	
			break;
		}
            
        default:
            p_token=SkipToken(p_token);
            break;
        }    
    }
    p_token=p_qb;
	end_of_file=false;
	// That's all the checksum names added.


	// Register the file name checksum so that FindChecksumName will be able to find it.
	// (Required if a script assert goes off that needs to print the source file name)
	AddChecksumName(Crc::GenerateCRCFromString(p_fileName),p_fileName);

	// Store the checksum of the file name so that if an assert goes off during parsing the file name can be printed.
	s_qb_being_parsed=Crc::GenerateCRCFromString(p_fileName);

	// Make sure we're using the script heap, cos we're about to create a bunch of scripty stuff.
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
	
    while (!end_of_file)
    {
        p_token=SkipEndOfLines(p_token);
		
        switch (*p_token)
        {
			case ESCRIPTTOKEN_NAME:
			{
				p_token=sCreateSymbolOfTheFormNameEqualsValue(p_token,p_fileName,assertIfDuplicateSymbols);
				break;    
			}
			case ESCRIPTTOKEN_KEYWORD_SCRIPT:
			{
				// Get the new script's name checksum ...
				++p_token; // Skip over the ESCRIPTTOKEN_KEYWORD_SCRIPT
				Dbg_MsgAssert(*p_token==ESCRIPTTOKEN_NAME,("\nKeyword 'script' must be followed by a name, line %d of %s",GetLineNumber(p_token),p_fileName));
				++p_token; // Skip over the ESCRIPTTOKEN_NAME
				uint32 name_checksum=Read4Bytes(p_token).mChecksum;
				p_token+=4; // Skip over the name checksum
	
	
				// Calculate the length of the script data in bytes.
				uint8 *p_script_data=p_token;
				p_token=SkipOverScript(p_token);
				uint32 script_data_size=p_token-p_script_data;
	
				
				// Calculate a checksum of the contents of the new script.
				// The purpose of the contents checksum is to prevent unnecessary restarting of CScript's
				// when a qb is reloaded. It may be that only one script in the qb has changed, so we only
				// want the CScript's that were referencing that script to restart.
				
				// Note: The LoadQB function in gel\scripting\file.cpp does the restarting of existing
				// CScripts by checking the mGotReloaded flag in the CSymbolTableEntry for the script.
				
				uint32 new_contents_checksum=CalculateScriptContentsChecksum(p_script_data);
				
				// Check to see if a script with this name exists already.			
				CSymbolTableEntry *p_existing_entry=LookUpSymbol(name_checksum);
				if (p_existing_entry)
				{
					if (assertIfDuplicateSymbols)
					{
						Dbg_MsgAssert(0,("The symbol '%s' is defined twice, in %s and %s",FindChecksumName(name_checksum),FindChecksumName(p_existing_entry->mSourceFileNameChecksum),p_fileName));
					}
				
					if (p_existing_entry->mType==ESYMBOLTYPE_QSCRIPT)
					{
						// A script with the same name does already exist.
						// Compare its contents checksum with the new one.
#ifdef __PLAT_NGC__
						uint32 header[(SCRIPT_HEADER_SIZE/4)];
						NsDMA::toMRAM( header, p_existing_entry->mScriptOffset, SCRIPT_HEADER_SIZE );
						uint32 old_contents_checksum=header[0];
#else
						Dbg_MsgAssert(p_existing_entry->mpScript,("NULL p_existing_entry->mpScript ??"));
						uint32 old_contents_checksum=*(uint32*)(p_existing_entry->mpScript);
#endif		// __PLAT_NGC__
						
						if (new_contents_checksum != old_contents_checksum)
						{
							// The contents of the new script differ from the old!
							// So remove the old one, and create the new.
							CleanUpAndRemoveSymbol(p_existing_entry);
							sCreateScriptSymbol(name_checksum,
												new_contents_checksum,
												p_script_data,
												script_data_size,
												p_fileName);
						}
						else
						{
							// The contents of the script have not changed!
							// So there's nothing to do.
						}
					}
					else
					{
						// Remove the existing symbol, whatever it is. (It isn't a script)
						CleanUpAndRemoveSymbol(p_existing_entry);
						// Create the new script symbol.
						sCreateScriptSymbol(name_checksum,new_contents_checksum,p_script_data,script_data_size,p_fileName);
					}
				}	
				else
				{
					// No symbol currently exists with this name, so create the new script symbol.
					sCreateScriptSymbol(name_checksum,new_contents_checksum,p_script_data,script_data_size,p_fileName);
				}			
				break;
			}
	
			case ESCRIPTTOKEN_CHECKSUM_NAME: 
				// These have been done already, so skip over.
				p_token=SkipToken(p_token);
				break;
				
			case ESCRIPTTOKEN_ENDOFFILE:
				end_of_file=true;
				break;
				
			default:
				Dbg_MsgAssert(0,("\nConfused by line %d of %s.\nExpected either a script definition (Script ... EndScript)\nor a constant definition (Something=...)",GetLineNumber(p_token),p_fileName));
				break;
        }    
    }

	Mem::Manager::sHandle().PopContext();
	s_qb_being_parsed=0;
}

// Used by the EditorCameraComponent when it checks polys to see if they are Kill polys
bool ScriptContainsName(uint8 *p_script, uint32 searchName)
{
	Dbg_MsgAssert(p_script,("NULL p_script"));

	//int num_names_found=0;
	//uint32 last_name=0;
	
	uint8 *p_token=p_script;
	while (*p_token!=ESCRIPTTOKEN_KEYWORD_ENDSCRIPT) 
	{
		switch (*p_token)
		{
			case ESCRIPTTOKEN_NAME:
			{
				++p_token;
				uint32 name=Read4Bytes(p_token).mChecksum;
				p_token+=4;
				if (name == searchName)
				{
					return true;
				}	
				
				//++num_names_found;
				//last_name=name;
				break;
			}	
				
			default:
				p_token=SkipToken(p_token);
				break;
		}
	}	
	
	/*
	// I put this in so that scripts that consist of a single call to another script will have that
	// scanned. This was to fix a bug where the kill poly over the swimming pool in Hawaii was not
	// being detected. However, this fix then meant that the kill polys around the hotel would get
	// detected, meaning that the cursor could never be got off the hotel. So commented it out
	// for the moment, since at least the first bug does not make the cursor get stuck.
	if (num_names_found==1)
	{
		Script::CScriptCache *p_script_cache=Script::CScriptCache::Instance();
		Dbg_MsgAssert(p_script_cache,("NULL p_script_cache"));
	
		p_script=p_script_cache->GetScript(last_name);
		if (p_script)
		{
			bool contains_name=ScriptContainsName(p_script, searchName);
			p_script_cache->DecrementScriptUsage(last_name);
			return contains_name;
		}
	}
	*/
	
	return false;
}

// Used by the EditorCameraComponent when it checks polys to see if they are Kill polys
bool ScriptContainsAnyOfTheNames(uint32 scriptName, uint32 *p_names, int numNames)
{
	Script::CScriptCache *p_script_cache=Script::CScriptCache::Instance();
	Dbg_MsgAssert(p_script_cache,("NULL p_script_cache"));

	uint8 *p_script=p_script_cache->GetScript(scriptName);
	Dbg_MsgAssert(p_script,("NULL p_script for %s",Script::FindChecksumName(scriptName)));

	bool contains_name=false;
	
	Dbg_MsgAssert(p_names,("NULL p_names"));
	uint32 *p_name=p_names;
	for (int i=0; i<numNames; ++i)
	{
		if (ScriptContainsName(p_script,*p_name))
		{
			//printf("Script %s contains %s\n",Script::FindChecksumName(scriptName),Script::FindChecksumName(*p_name));
			contains_name=true;
			break;
		}
		++p_name;
	}
	
	p_script_cache->DecrementScriptUsage(scriptName);
	
	return contains_name;		
}

} // namespace Script



