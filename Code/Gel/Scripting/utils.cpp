///////////////////////////////////////////////////////////////////////////////////////
//
// utils.cpp		KSH 22 Oct 2001
//
// Misc script utility functions.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <gel/scripting/utils.h>
#include <gel/scripting/checksum.h>
#include <gel/scripting/component.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/parse.h>
#include <gel/scripting/symboltable.h>
#include <gel/scripting/string.h>

// Some defines that affect how PrintContents works:

// If defined, then if a structure contains a reference to a global structure, PrintContents
// will print the contents of that structure too.
// (Added this define so that it can be disabled for viewing the mem card career-save structure,
// which has lots of references and takes ages to print if they are all expanded)
#define EXPAND_GLOBAL_STRUCTURE_REFERENCES

// Enables a for-loop to slow down PrintContents so that printf can catch up.
// (Needed for viewing big mem card structures)
//#define SLOW_DOWN_PRINTCONTENTS

namespace Script
{

static uint8 *sWriteCompressedName(uint8 *p_buffer, uint8 SymbolType, uint32 NameChecksum);
static uint32 sIntegerWriteToBuffer(uint32 Name, int val, uint8 *p_buffer, uint32 BufferSize);
static uint32 sFloatWriteToBuffer(uint32 Name, float val, uint8 *p_buffer, uint32 BufferSize);
static uint32 sChecksumWriteToBuffer(uint32 Name, uint32 Checksum, uint8 *p_buffer, uint32 BufferSize);
static uint32 sStringWriteToBuffer(uint32 Name, const char *pString, uint8 *p_buffer, uint32 BufferSize);
static uint32 sLocalStringWriteToBuffer(uint32 Name, const char *pString, uint8 *p_buffer, uint32 BufferSize);
static uint32 sPairWriteToBuffer(uint32 Name, CPair *pPair, uint8 *p_buffer, uint32 BufferSize);
static uint32 sVectorWriteToBuffer(uint32 Name, CVector *pVector, uint8 *p_buffer, uint32 BufferSize);
static uint32 sStructureWriteToBuffer(uint32 Name, CStruct *pStructure, uint8 *p_buffer, uint32 BufferSize, EAssertType assert);
static uint32 sArrayWriteToBuffer(uint32 Name, CArray *pArray, uint8 *p_buffer, uint32 BufferSize, EAssertType assert);

#ifdef __NOPT_ASSERT__
static void   sDoIndent(int indent);

static void sDoIndent(int indent)
{
	for (int i=0; i<indent; ++i)
	{
		printf(" ");
	}
}
#endif

void PrintContents(const CArray *p_array, int indent)
{
#ifdef __NOPT_ASSERT__	
	Dbg_MsgAssert(p_array,("NULL p_array"));
	int size=p_array->GetSize();
	ESymbolType type=p_array->GetType();
	
	if (!indent)
	{
		printf("Contents of Array=\n");
	}
	
	sDoIndent(indent);
	printf("[\n");
	indent+=3;
	
	int int_column_count=0;
	for (int i=0; i<size; ++i)
	{
		if (type!=ESYMBOLTYPE_STRUCTURE && type!=ESYMBOLTYPE_ARRAY)
		{
			if (type!=ESYMBOLTYPE_INTEGER || (type==ESYMBOLTYPE_INTEGER && int_column_count==0))
			{
				sDoIndent(indent);
			}	
		}	
		
        switch (type)
        {
        case ESYMBOLTYPE_INTEGER:
			if (i<size-1)
			{
				printf("%d,",p_array->GetInteger(i));
			}
			else
			{
				printf("%d\n",p_array->GetInteger(i));
			}	
			++int_column_count;
			if (int_column_count==15)
			{
				printf("\n");
				int_column_count=0;
			}
            break;
        case ESYMBOLTYPE_FLOAT:
            printf("%f\n",p_array->GetFloat(i));
            break;
        case ESYMBOLTYPE_STRING:
		{
			const char *p_string=p_array->GetString(i);
			if (p_string)
			{
				printf("\"%s\"\n",p_string);
			}	
			else
			{
				printf("NULL string\n");
			}	
            break;
		}	
        case ESYMBOLTYPE_LOCALSTRING:
		{
			const char *p_string=p_array->GetLocalString(i);
			if (p_string)
			{
				printf("'%s'\n",p_string);
			}	
			else
			{
				printf("NULL local string\n");
			}	
            break;
		}	
        case ESYMBOLTYPE_PAIR:
		{
			CPair *p_pair=p_array->GetPair(i);
			if (p_pair)
			{
				printf("(%f,%f)\n",p_pair->mX,p_pair->mY);
			}	
			else
			{
				printf("NULL pair\n");
			}	
            break;
		}	
        case ESYMBOLTYPE_VECTOR:
		{
			CVector *p_vector=p_array->GetVector(i);
			if (p_vector)
			{
				printf("(%f,%f,%f)\n",p_vector->mX,p_vector->mY,p_vector->mZ);
			}	
			else
			{
				printf("NULL vector\n");
			}	
            break;
		}	
        case ESYMBOLTYPE_NAME:
            printf("%s\n",FindChecksumName(p_array->GetChecksum(i)));
            break;
        case ESYMBOLTYPE_STRUCTURE:
		{
			CStruct *p_structure=p_array->GetStructure(i);
			if (p_structure)
			{
				PrintContents(p_structure,indent);
			}	
			else
			{
				printf("NULL structure\n");
			}	
            break;
		}	
		case ESYMBOLTYPE_ARRAY:
		{
			CArray *p_a=p_array->GetArray(i);
			if (p_a)
			{
				PrintContents(p_a,indent);
			}	
			else
			{
				printf("NULL array\n");
			}	
            break;
		}	
			
        default:
            Dbg_MsgAssert(0,("Bad array type"));
            break;
        }

		#ifdef SLOW_DOWN_PRINTCONTENTS
		// A delay to let printf catch up so that it doesn't skip stuff when printing big arrays.		
		for (int i=0; i<10000; ++i);
		#endif
    }
    sDoIndent(indent-3);
	printf("]\n");
#endif	
}

void PrintContents(const CStruct *p_structure, int indent)
{
#ifdef __NOPT_ASSERT__
	Dbg_MsgAssert(p_structure,("NULL p_structure"));
	
	if (!indent)
	{
		printf("Contents of CStruct=\n");
	}
	
	sDoIndent(indent);
	printf("{\n");
	indent+=3;
		
    CComponent *p_comp=p_structure->GetNextComponent(NULL);
    while (p_comp)
    {
		sDoIndent(indent);
		
		if (p_comp->mNameChecksum)
		{
			printf("%s=",FindChecksumName(p_comp->mNameChecksum));
		}	
            
        switch (p_comp->mType)
        {
        case ESYMBOLTYPE_INTEGER:
            printf("%d\n",p_comp->mIntegerValue);
            break;
        case ESYMBOLTYPE_FLOAT:
            printf("%f\n",p_comp->mFloatValue);
            break;
        case ESYMBOLTYPE_STRING:
            printf("\"%s\"\n",p_comp->mpString);
            break;
        case ESYMBOLTYPE_LOCALSTRING:
            printf("'%s'\n",p_comp->mpLocalString);
            break;
        case ESYMBOLTYPE_PAIR:
            printf("(%f,%f)\n",p_comp->mpPair->mX,p_comp->mpPair->mY);
            break;
        case ESYMBOLTYPE_VECTOR:
            printf("(%f,%f,%f)\n",p_comp->mpVector->mX,p_comp->mpVector->mY,p_comp->mpVector->mZ);
            break;
        case ESYMBOLTYPE_STRUCTURE:
			printf("\n");
			PrintContents(p_comp->mpStructure,indent);
            break;
        case ESYMBOLTYPE_NAME:
            printf("%s\n",FindChecksumName(p_comp->mChecksum));
			
			#ifdef EXPAND_GLOBAL_STRUCTURE_REFERENCES
			if (p_comp->mNameChecksum==0)
			{
				// It's an un-named name. Maybe it's a global structure ... 
				// If so, print its contents too, which is handy for debugging.
			    CSymbolTableEntry *p_entry=Resolve(p_comp->mChecksum);
				if (p_entry && p_entry->mType==ESYMBOLTYPE_STRUCTURE)
				{
					sDoIndent(indent);
					printf("... Defined in %s ...\n",FindChecksumName(p_entry->mSourceFileNameChecksum));
					Dbg_MsgAssert(p_entry->mpStructure,("NULL p_entry->mpStructure"));
					PrintContents(p_entry->mpStructure,indent);
				}
			}		
			#endif
            break;
        case ESYMBOLTYPE_QSCRIPT:
			printf("(A script)\n"); // TODO
			break;
		case ESYMBOLTYPE_ARRAY:
			printf("\n");
			PrintContents(p_comp->mpArray,indent);
            break;
        default:
			printf("Component of type '%s', value 0x%08x\n",GetTypeName(p_comp->mType),p_comp->mUnion);
            //Dbg_MsgAssert(0,("Bad p_comp->Type"));
            break;
        }
        p_comp=p_structure->GetNextComponent(p_comp);
		
		#ifdef SLOW_DOWN_PRINTCONTENTS
		// A delay to let printf catch up so that it doesn't skip stuff when printing big arrays.		
		for (int i=0; i<1000000; ++i);
		#endif
    }
	
    sDoIndent(indent-3);
	printf("}\n");
#endif
}

static uint8 *sWriteCompressedName(uint8 *p_buffer, uint8 symbolType, uint32 nameChecksum)
{
	#ifdef __PLAT_WN32__
	// If compiling on PC, the lookup table will not exist, so do no compression.
	*p_buffer++=symbolType;
	return Write4Bytes(p_buffer,nameChecksum);
	#else
	
	// Check if the checksum is in the small array.
	CArray *p_table=GetArray(0x35115a20/*WriteToBuffer_CompressionLookupTable_8*/);
	int size=p_table->GetSize();
	Dbg_MsgAssert(size<256,("Size of WriteToBuffer_CompressionLookupTable_8 too big"));
	
	for (int i=0; i<size; ++i)
	{
		if (p_table->GetChecksum(i)==nameChecksum)
		{
			// It is in the array! So write out a 1 byte index.
			*p_buffer++=symbolType | MASK_8_BIT_NAME_LOOKUP;
			*p_buffer++=i;
			return p_buffer;
		}
	}		
	
	// Check if the checksum is in the big array.
	p_table=GetArray(0x25231f42/*WriteToBuffer_CompressionLookupTable_16*/);
	size=p_table->GetSize();
	Dbg_MsgAssert(size<65536,("Size of WriteToBuffer_CompressionLookupTable_16 too big"));
	
	for (int i=0; i<size; ++i)
	{
		if (p_table->GetChecksum(i)==nameChecksum)
		{
			// It is in the array! So write out a 2 byte index.
			*p_buffer++=symbolType | MASK_16_BIT_NAME_LOOKUP;
			return Write2Bytes(p_buffer,i);
		}
	}		
	
	// Oh well, it is not in either array, so write out the whole 4 byte checksum.
	*p_buffer++=symbolType;
	return Write4Bytes(p_buffer,nameChecksum);
	#endif
}

static uint32 sIntegerWriteToBuffer(uint32 name, int val, uint8 *p_buffer, uint32 bufferSize)
{
	uint8 *p_buffer_before=p_buffer;
		
	// Choose what type of symbol to use so as to minimize the space used.
	
	ESymbolType symbol_type=ESYMBOLTYPE_INTEGER; // Default to a full 4 bytes.
	
	if (val==0)
	{
		symbol_type=ESYMBOLTYPE_ZERO_INTEGER;
	}
	else if (val>=0 && val<=255)
	{
		symbol_type=ESYMBOLTYPE_UNSIGNED_INTEGER_ONE_BYTE;
	}
	else if (val>=0 && val<=65535)	
	{
		symbol_type=ESYMBOLTYPE_UNSIGNED_INTEGER_TWO_BYTES;
	}
	else if (val>=-128 && val<=127)
	{
		symbol_type=ESYMBOLTYPE_INTEGER_ONE_BYTE;
	}
	else if (val>=-32768 && val<=32767)
	{
		symbol_type=ESYMBOLTYPE_INTEGER_TWO_BYTES;
	}

	
	uint8 p_temp[20];
	uint8 *p_end=p_temp;
	 
	switch (symbol_type)
	{
		case ESYMBOLTYPE_INTEGER:
			// Write out the component into p_temp
			p_end=sWriteCompressedName(p_temp,symbol_type,name);
			p_end=Write4Bytes(p_end,(uint32)val);
			break;
		
		case ESYMBOLTYPE_INTEGER_ONE_BYTE:
		case ESYMBOLTYPE_UNSIGNED_INTEGER_ONE_BYTE:
			// Write out the component into p_temp
			p_end=sWriteCompressedName(p_temp,symbol_type,name);
			*p_end++=(uint8)val;
			break;
			
		case ESYMBOLTYPE_INTEGER_TWO_BYTES:
		case ESYMBOLTYPE_UNSIGNED_INTEGER_TWO_BYTES:
			// Write out the component.
			p_end=sWriteCompressedName(p_temp,symbol_type,name);
			p_end=Write2Bytes(p_end,(uint16)val);
			break;
			
		case ESYMBOLTYPE_ZERO_INTEGER:
			// Write out the component.
			p_end=sWriteCompressedName(p_temp,symbol_type,name);
			break;
			
		default:
			Dbg_MsgAssert(0,("Bad symbol_type"));
			break;
	}

	uint32 bytes_written_to_temp=p_end-p_temp;
	
	// Check that there is enough space before doing any writing.
	if (bufferSize < bytes_written_to_temp)
	{
		return 0;
	}

	uint8 *p_source=p_temp;
	for (uint32 i=0; i<bytes_written_to_temp; ++i)
	{
		*p_buffer++=*p_source++;
	}	
	
	return p_buffer-p_buffer_before;		
}

static uint32 sFloatWriteToBuffer(uint32 name, float val, uint8 *p_buffer, uint32 bufferSize)
{
	uint8 *p_buffer_before=p_buffer;
	
	float abs_val=val;
	if (abs_val<0.0f) abs_val=-abs_val;
	
	// Check if the float value is small enough to be considered zero.
	// If so, save space by using the ESYMBOLTYPE_ZERO_FLOAT token.
	if (abs_val<0.0001)
	{
		// Check that there is enough space before doing any writing.
		if (bufferSize<5)
		{
			return 0;
		}
		
		// Write out the component.
		p_buffer=sWriteCompressedName(p_buffer,ESYMBOLTYPE_ZERO_FLOAT,name);
	}
	else
	{
		// Check that there is enough space before doing any writing.
		if (bufferSize<9)
		{
			return 0;
		}
		
		// Write out the component.
		p_buffer=sWriteCompressedName(p_buffer,ESYMBOLTYPE_FLOAT,name);
		p_buffer=Write4Bytes(p_buffer,val);
	}	
	return p_buffer-p_buffer_before;
}

static uint32 sChecksumWriteToBuffer(uint32 name, uint32 checksum, uint8 *p_buffer, uint32 bufferSize)
{
	uint8 *p_buffer_before=p_buffer;
	
	// Check that there is enough space before doing any writing.
	if (bufferSize<9)
	{
		return 0;
	}
		
	// Write out the component.
	p_buffer=sWriteCompressedName(p_buffer,ESYMBOLTYPE_NAME,name);
	p_buffer=Write4Bytes(p_buffer,checksum);
	return p_buffer-p_buffer_before;
}

static uint32 sStringWriteToBuffer(uint32 name, const char *p_string, uint8 *p_buffer, uint32 bufferSize)
{
	Dbg_MsgAssert(p_string,("NULL p_string sent to sStringWriteToBuffer"));
	uint8 *p_buffer_before=p_buffer;
		
	uint32 Len=strlen(p_string);
	// Check that there is enough space before doing any writing.
	// The +6 is for the type, the name, and the string terminating 0.
	if (bufferSize<Len+6)
	{
		return 0;
	}	
	
	// Write out the component.
	p_buffer=sWriteCompressedName(p_buffer,ESYMBOLTYPE_STRING,name);
	for (uint32 i=0; i<Len; ++i)
	{
		*p_buffer++=*p_string++;
	}
	*p_buffer++=0;	
	
	return p_buffer-p_buffer_before;
}

static uint32 sLocalStringWriteToBuffer(uint32 name, const char *p_string, uint8 *p_buffer, uint32 bufferSize)
{
	Dbg_MsgAssert(p_string,("NULL p_string sent to sLocalStringWriteToBuffer"));
	uint8 *p_buffer_before=p_buffer;
	
	uint32 len=strlen(p_string);
	// Check that there is enough space before doing any writing.
	// The +6 is for the type, the name, and the string terminating 0.
	if (bufferSize<len+6)
	{
		return 0;
	}	
	
	// Write out the component.
	p_buffer=sWriteCompressedName(p_buffer,ESYMBOLTYPE_LOCALSTRING,name);
	for (uint32 i=0; i<len; ++i)
	{
		*p_buffer++=*p_string++;
	}
	*p_buffer++=0;	
	
	return p_buffer-p_buffer_before;
}

static uint32 sPairWriteToBuffer(uint32 name, CPair *p_pair, uint8 *p_buffer, uint32 bufferSize)
{
	Dbg_MsgAssert(p_pair,("NULL p_pair sent to sPairWriteToBuffer"));
	uint8 *p_buffer_before=p_buffer;
	
	// Check that there is enough space before doing any writing.
	if (bufferSize<13)
	{
		return 0;
	}
		
	// Write out the component.
	p_buffer=sWriteCompressedName(p_buffer,ESYMBOLTYPE_PAIR,name);
	p_buffer=Write4Bytes(p_buffer,p_pair->mX);
	p_buffer=Write4Bytes(p_buffer,p_pair->mY);
	
	return p_buffer-p_buffer_before;
}

static uint32 sVectorWriteToBuffer(uint32 name, CVector *p_vector, uint8 *p_buffer, uint32 bufferSize)
{
	Dbg_MsgAssert(p_vector,("NULL p_vector sent to sVectorWriteToBuffer"));
	uint8 *p_buffer_before=p_buffer;
	
	// Check that there is enough space before doing any writing.
	if (bufferSize<17)
	{
		return 0;
	}
		
	// Write out the component.
	p_buffer=sWriteCompressedName(p_buffer,ESYMBOLTYPE_VECTOR,name);
	p_buffer=Write4Bytes(p_buffer,p_vector->mX);
	p_buffer=Write4Bytes(p_buffer,p_vector->mY);
	p_buffer=Write4Bytes(p_buffer,p_vector->mZ);
	
	return p_buffer-p_buffer_before;
}

static uint32 sStructureWriteToBuffer(uint32 name, CStruct *p_structure, uint8 *p_buffer, uint32 bufferSize, EAssertType assert)
{
	Dbg_MsgAssert(p_structure,("NULL p_structure sent to sStructureWriteToBuffer"));
	uint8 *p_buffer_before=p_buffer;
	
	// Check that there is enough space before doing any writing.
	if (bufferSize<5)
	{
		return 0;
	}
		
	// Write out the type and name
	p_buffer=sWriteCompressedName(p_buffer,ESYMBOLTYPE_STRUCTURE,name);
	int name_bytes_written=p_buffer-p_buffer_before;
	
	// That's name_bytes_written bytes written out successfully, but maybe the writing out of the structure will fail ...
	
	uint32 structure_bytes_written=WriteToBuffer(p_structure,p_buffer,bufferSize-name_bytes_written,assert);
	// If writing out the structure failed, return 0.
	if (!structure_bytes_written)
	{
		return 0;
	}
	
	// Otherwise return the total bytes written.
	return name_bytes_written+structure_bytes_written;
}

static uint32 sArrayWriteToBuffer(uint32 name, CArray *p_array, uint8 *p_buffer, uint32 bufferSize, EAssertType assert)
{
	Dbg_MsgAssert(p_array,("NULL p_array sent to sArrayWriteToBuffer"));
	uint8 *p_buffer_before=p_buffer;
	
	// Check that there is enough space before doing any writing.
	if (bufferSize<5)
	{
		return 0;
	}

	// Write out the type and name
	p_buffer=sWriteCompressedName(p_buffer,ESYMBOLTYPE_ARRAY,name);
	int name_bytes_written=p_buffer-p_buffer_before;
	
	// That's name_bytes_written bytes written out successfully, but maybe the writing out of the array will fail ...
	
	uint32 array_bytes_written=WriteToBuffer(p_array,p_buffer,bufferSize-name_bytes_written,assert);
	// If writing out the array failed, return 0.
	if (!array_bytes_written)
	{
		return 0;
	}
	
	// Otherwise return the total bytes written.
	return name_bytes_written+array_bytes_written;
}	

// Writes the contents of the structure to a buffer.
// The information is outputted in a byte-packed format, so p_buffer does
// not need to be aligned.
// The buffer can then be used to regenerate the original structure by
// passing it to ReadFromBuffer.
// Passed the size of the buffer so that it can check if there is enough space.
// Returns the number of bytes that it actually wrote.
//
// If there was not enough space, and assert is NO_ASSERT, it will return a count of 0.
//
uint32 WriteToBuffer(CStruct *p_structure, uint8 *p_buffer, uint32 bufferSize, EAssertType assert)
{
	Dbg_MsgAssert(p_buffer,("NULL p_buffer sent to WriteToBuffer"));
	uint32 bytes_left=bufferSize;

	// Scan through the components adding each to the buffer.	
    CComponent *p_comp=NULL;
	if (p_structure)
	{
		p_comp=p_structure->GetNextComponent(NULL);
	}
		
    while (p_comp)
    {
		uint32 component_bytes_written=0;
		
        switch (p_comp->mType)
        {
        case ESYMBOLTYPE_INTEGER:
			component_bytes_written=sIntegerWriteToBuffer(p_comp->mNameChecksum,p_comp->mIntegerValue,p_buffer,bytes_left);
			break;
			
        case ESYMBOLTYPE_FLOAT:
			component_bytes_written=sFloatWriteToBuffer(p_comp->mNameChecksum,p_comp->mFloatValue,p_buffer,bytes_left);
			break;

        case ESYMBOLTYPE_NAME:
			component_bytes_written=sChecksumWriteToBuffer(p_comp->mNameChecksum,p_comp->mChecksum,p_buffer,bytes_left);
			break;
			
        case ESYMBOLTYPE_STRING:
			component_bytes_written=sStringWriteToBuffer(p_comp->mNameChecksum,p_comp->mpString,p_buffer,bytes_left);
			break;

        case ESYMBOLTYPE_LOCALSTRING:
			component_bytes_written=sLocalStringWriteToBuffer(p_comp->mNameChecksum,p_comp->mpLocalString,p_buffer,bytes_left);
			break;
			
        case ESYMBOLTYPE_PAIR:
			component_bytes_written=sPairWriteToBuffer(p_comp->mNameChecksum,p_comp->mpPair,p_buffer,bytes_left);
			break;
			
        case ESYMBOLTYPE_VECTOR:
			component_bytes_written=sVectorWriteToBuffer(p_comp->mNameChecksum,p_comp->mpVector,p_buffer,bytes_left);
			break;

        case ESYMBOLTYPE_STRUCTURE:
			component_bytes_written=sStructureWriteToBuffer(p_comp->mNameChecksum,p_comp->mpStructure,p_buffer,bytes_left,assert);
			break;

        case ESYMBOLTYPE_ARRAY:
			component_bytes_written=sArrayWriteToBuffer(p_comp->mNameChecksum,p_comp->mpArray,p_buffer,bytes_left,assert);
			break;
			
		case ESYMBOLTYPE_QSCRIPT:	
			component_bytes_written=sChecksumWriteToBuffer(p_comp->mNameChecksum,CRCD(0xb9c4f664,"InlineScript"),p_buffer,bytes_left);
            break;
			
        default:
            Dbg_MsgAssert(0,("Component type of '%s' is not supported in WriteToBuffer",GetTypeName(p_comp->mType)));
            break;
        }
		
		// If any of the above writes failed due to lack of space, then bail out too.
		if (!component_bytes_written)
		{
			if (assert)
			{
				Dbg_MsgAssert(0,("WriteToBuffer: Buffer not big enough"));
			}	
			return 0;
		}
			
		bytes_left-=component_bytes_written;
		p_buffer+=component_bytes_written;
		
        p_comp=p_structure->GetNextComponent(p_comp);
    }

	// Add the terminating ESYMBOLTYPE_NONE
	if (!bytes_left)
	{
		// Aargh! Not enough room left for the final byte!
		if (assert)
		{
			Dbg_MsgAssert(0,("WriteToBuffer: Buffer not big enough"));
		}	
		return 0;
	}	
	*p_buffer++=ESYMBOLTYPE_NONE;
	--bytes_left;
	
	// Return how many bytes got written.
	return bufferSize-bytes_left;
}

// Calculates how big a buffer will need to be to hold a structure using WriteToBuffer.
uint32 CalculateBufferSize(CStruct *p_structure, uint32 tempBufferSize)
{
	Dbg_MsgAssert(p_structure,("NULL p_structure"));

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap()); // Use the temporary heap
	
	uint8 *p_temp=(uint8*)Mem::Malloc(tempBufferSize);
	Dbg_MsgAssert(p_temp,("Could not allocate temporary buffer"));
	uint32 space_required=WriteToBuffer(p_structure,p_temp,tempBufferSize);
	Mem::Free(p_temp);
	Mem::Manager::sHandle().PopContext();

	return space_required;
}

// Sets the structure's contents using the passed buffer, which was generated
// by WriteToBuffer.
// If the structure contained anything to begin with, it will get cleared.	
//
// Returns a pointer to after the terminating ESYMBOLTYPE_NONE, required
// when this function calls itself.
uint8 *ReadFromBuffer(CStruct *p_structure, uint8 *p_buffer)
{
	Dbg_MsgAssert(p_structure,("NULL p_structure"));
	Dbg_MsgAssert(p_buffer,("NULL p_buffer sent to ReadFromBuffer"));
	float zero_float=0.0f;

	// First clear anything currently in the structure.
	p_structure->Clear();

	// Make sure we're using the script heap.
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
	
	// Scan through the buffer adding the components until ESYMBOLTYPE_NONE is reached.
	while (true)
    {
		float x,y,z;
		const char *p_string;
		CStruct *p_struct;
		CArray *p_array;
		
		// Get the type and the name checksum.
		ESymbolType type=(ESymbolType)*p_buffer++;
		if (type==ESYMBOLTYPE_NONE)
		{
			// All done.
			break;
		}

		// Get the name checksum, which may be stored as an index into a table of checksums
		// to save space.
		uint32 name=0;
		if (type&MASK_8_BIT_NAME_LOOKUP)			
		{
			Dbg_MsgAssert(!(type&MASK_16_BIT_NAME_LOOKUP),("Eh? Both lookup-table flags set ?"));
			
			#ifdef __PLAT_WN32__
			// The lookup table is not loaded when compiling on PC
			name=CRCD(0xef5f3f41,"CompressedName");
			#else
			CArray *p_table=GetArray(0x35115a20/*WriteToBuffer_CompressionLookupTable_8*/);
			name=p_table->GetChecksum(*p_buffer);
			#endif
			++p_buffer;
		}
		else if (type&MASK_16_BIT_NAME_LOOKUP)			
		{
			#ifdef __PLAT_WN32__
			name=CRCD(0xef5f3f41,"CompressedName");
			#else
			CArray *p_table=GetArray(0x25231f42/*WriteToBuffer_CompressionLookupTable_16*/);
			name=p_table->GetChecksum(Read2Bytes(p_buffer).mUInt);
			#endif
			p_buffer+=2;
		}
		else
		{
			// It's not stored as an index, but as the full 4 byte checksum.
			name=Read4Bytes(p_buffer).mChecksum;
			p_buffer+=4;
		}
		type=(ESymbolType)( ((uint32)type) & ~(MASK_8_BIT_NAME_LOOKUP | MASK_16_BIT_NAME_LOOKUP) );
			
		
        switch (type)
        {
        case ESYMBOLTYPE_INTEGER:
			p_structure->AddInteger(name,Read4Bytes(p_buffer).mInt);
			p_buffer+=4;
            break;
        case ESYMBOLTYPE_FLOAT:
			p_structure->AddFloat(name,Read4Bytes(p_buffer).mFloat);
			p_buffer+=4;
            break;
        case ESYMBOLTYPE_NAME:
			p_structure->AddChecksum(name,Read4Bytes(p_buffer).mChecksum);
			p_buffer+=4;
            break;
		case ESYMBOLTYPE_ZERO_FLOAT:
			p_structure->AddFloat(name,zero_float);
			break;	
		case ESYMBOLTYPE_ZERO_INTEGER:
			p_structure->AddInteger(name,0);
			break;	
		case ESYMBOLTYPE_INTEGER_ONE_BYTE:
			p_structure->AddInteger(name,*(sint8*)p_buffer++);
			break;
		case ESYMBOLTYPE_UNSIGNED_INTEGER_ONE_BYTE:
			p_structure->AddInteger(name,*p_buffer++);
			break;
		case ESYMBOLTYPE_INTEGER_TWO_BYTES:
			p_structure->AddInteger(name,Read2Bytes(p_buffer).mInt);
			p_buffer+=2;
			break;	
		case ESYMBOLTYPE_UNSIGNED_INTEGER_TWO_BYTES:
			p_structure->AddInteger(name,Read2Bytes(p_buffer).mUInt);
			p_buffer+=2;
			break;	
        case ESYMBOLTYPE_STRING:
			p_string=(const char *)p_buffer;
			p_structure->AddString(name,p_string);
			p_buffer+=strlen(p_string);
			++p_buffer; // Skip over the terminator too.
            break;
        case ESYMBOLTYPE_LOCALSTRING:
			p_string=(const char *)p_buffer;
			p_structure->AddLocalString(name,p_string);
			p_buffer+=strlen(p_string);
			++p_buffer; // Skip over the terminator too.
            break;
        case ESYMBOLTYPE_PAIR:
			x=Read4Bytes(p_buffer).mFloat;
			p_buffer+=4;
			y=Read4Bytes(p_buffer).mFloat;
			p_buffer+=4;
			p_structure->AddPair(name,x,y);
            break;
        case ESYMBOLTYPE_VECTOR:
			x=Read4Bytes(p_buffer).mFloat;
			p_buffer+=4;
			y=Read4Bytes(p_buffer).mFloat;
			p_buffer+=4;
			z=Read4Bytes(p_buffer).mFloat;
			p_buffer+=4;
			p_structure->AddVector(name,x,y,z);
            break;
		case ESYMBOLTYPE_STRUCTURE:
			// Create a new structure, and fill it in by calling this function recursively.
			p_struct=new CStruct;
			p_buffer=ReadFromBuffer(p_struct,p_buffer);
			// Add the new component.
			p_structure->AddStructurePointer(name,p_struct);
			break;	
		case ESYMBOLTYPE_ARRAY:
			// Create a new array, and fill it in using the array's ReadFromBuffer.
			p_array=new CArray;
			p_buffer=ReadFromBuffer(p_array,p_buffer);
			// Add the new component.
			p_structure->AddArrayPointer(name,p_array);
			break;	
        default:
            Dbg_MsgAssert(0,("Unsupported component type of '%s' encountered in ReadFromBuffer",GetTypeName(type)));
            break;
        }
    }

	Mem::Manager::sHandle().PopContext();
	
	return p_buffer;
}

// Writes the contents of the array to a buffer.
// The information is outputted in a byte-packed format, so p_buffer does
// not need to be aligned.
// The buffer can then be used to regenerate the original array by
// passing it to ReadFromBuffer.
// Passed the size of the buffer so that it can check if there is enough space.
// Returns the number of bytes that it actually wrote.
//
// If there was not enough space, and assert is false, it will return a count of 0.
//
uint32 WriteToBuffer(CArray *p_array, uint8 *p_buffer, uint32 bufferSize, EAssertType assert)
{
	Dbg_MsgAssert(p_array,("NULL p_array"));
	Dbg_MsgAssert(p_buffer,("NULL p_buffer sent to WriteToBuffer"));
	uint32 bytes_left=bufferSize;

	if (bytes_left<3)
	{
		if (assert)
		{
			Dbg_MsgAssert(0,("WriteToBuffer: Buffer not big enough"));
		}	
		return 0;
	}
	
	ESymbolType type=p_array->GetType();
	uint32 size=p_array->GetSize();
	
	*p_buffer++=type;

	// Easy to change WriteToBuffer to support 4 byte sizes, but keeping as 2 for the moment for
	// backwards compatibility.
	Dbg_MsgAssert(size<0x10000,("Size of array too big, currently only 2 bytes used to store size in WriteToBuffer ..."));
	p_buffer=Write2Bytes(p_buffer,size);
	bytes_left-=3;
	
	switch (type)
	{
        case ESYMBOLTYPE_INTEGER:
        case ESYMBOLTYPE_FLOAT:
        case ESYMBOLTYPE_NAME:
		{
			if (size*4>bytes_left)
			{
				if (assert)
				{
					Dbg_MsgAssert(0,("WriteToBuffer: Buffer not big enough"));
				}	
				return 0;
			}
			
			uint32 *p_data=p_array->GetArrayPointer();
			for (uint32 i=0; i<size; ++i)
			{
				p_buffer=Write4Bytes(p_buffer,*p_data++);
			}
			
			bytes_left-=size*4;
			break;
		}
			
        case ESYMBOLTYPE_STRING:
        case ESYMBOLTYPE_LOCALSTRING:
		{
			char **pp_strings=(char**)p_array->GetArrayPointer();
			for (uint32 i=0; i<size; ++i)
			{
				const char *p_string=*pp_strings++;
				Dbg_MsgAssert(p_string,("NULL p_string for element %d when attempting to WriteToBuffer",i));

				while (*p_string)
				{
					if (bytes_left)
					{
						*p_buffer++=*p_string++;
						--bytes_left;
					}
					else
					{
						if (assert)
						{
							Dbg_MsgAssert(0,("WriteToBuffer: Buffer not big enough"));
						}	
						return 0;
					}
				}
				if (bytes_left)
				{
					*p_buffer++=0;
					--bytes_left;
				}
				else
				{
					if (assert)
					{
						Dbg_MsgAssert(0,("WriteToBuffer: Buffer not big enough"));
					}	
					return 0;
				}	
			}
			break;
		}
			
        case ESYMBOLTYPE_PAIR:
		{
			if (size*8>bytes_left)
			{
				if (assert)
				{
					Dbg_MsgAssert(0,("WriteToBuffer: Buffer not big enough"));
				}	
				return 0;
			}
			
			CPair **pp_pairs=(CPair**)p_array->GetArrayPointer();
			Dbg_MsgAssert(pp_pairs,("NULL pp_pairs ?"));
			
			for (uint32 i=0; i<size; ++i)
			{
				CPair *p_pair=*pp_pairs;
				Dbg_MsgAssert(p_pair,("NULL p_pair for element %d when attempting to WriteToBuffer",i));			
				p_buffer=Write4Bytes(p_buffer,p_pair->mX);
				p_buffer=Write4Bytes(p_buffer,p_pair->mY);
				++pp_pairs;
			}
			
			bytes_left-=size*8;
			break;
		}
			
        case ESYMBOLTYPE_VECTOR:
		{
			if (size*12>bytes_left)
			{
				if (assert)
				{
					Dbg_MsgAssert(0,("WriteToBuffer: Buffer not big enough"));
				}	
				return 0;
			}
			
			CVector **pp_vectors=(CVector**)p_array->GetArrayPointer();
			Dbg_MsgAssert(pp_vectors,("NULL pp_vectors ?"));
			
			for (uint32 i=0; i<size; ++i)
			{
				CVector *p_vector=*pp_vectors;
				Dbg_MsgAssert(p_vector,("NULL p_vector for element %d when attempting to WriteToBuffer",i));			
			
				p_buffer=Write4Bytes(p_buffer,p_vector->mX);
				p_buffer=Write4Bytes(p_buffer,p_vector->mY);
				p_buffer=Write4Bytes(p_buffer,p_vector->mZ);
				++pp_vectors;
			}
			
			bytes_left-=size*12;
			break;
		}
		
        case ESYMBOLTYPE_STRUCTURE:
		{
			CStruct **pp_structures=(CStruct**)p_array->GetArrayPointer();
			Dbg_MsgAssert(pp_structures,("NULL pp_structures ?"));
			
			for (uint32 i=0; i<size; ++i)
			{
				CStruct *p_structure=*pp_structures++;
				Dbg_MsgAssert(p_structure,("NULL p_structure for element %d when attempting to WriteToBuffer",i));			
				
				uint32 bytes_written=WriteToBuffer(p_structure,p_buffer,bytes_left,assert);
				if (!bytes_written)
				{
					if (assert)
					{
						Dbg_MsgAssert(0,("WriteToBuffer: Buffer not big enough"));
					}	
					return 0;
				}	
				else
				{
					bytes_left-=bytes_written;
					p_buffer+=bytes_written;
				}	
			}
			break;
		}
		
        case ESYMBOLTYPE_ARRAY:
		{
			CArray **pp_arrays=(CArray**)p_array->GetArrayPointer();
			Dbg_MsgAssert(pp_arrays,("NULL pp_arrays ?"));
			
			for (uint32 i=0; i<size; ++i)
			{
				CArray *p_array_element=*pp_arrays++;
				Dbg_MsgAssert(p_array_element,("NULL p_array_element for element %d when attempting to WriteToBuffer",i));			
				
				uint32 bytes_written=WriteToBuffer(p_array_element,p_buffer,bytes_left,assert);
				if (!bytes_written)
				{
					if (assert)
					{
						Dbg_MsgAssert(0,("WriteToBuffer: Buffer not big enough"));
					}	
					return 0;
				}	
				else
				{
					bytes_left-=bytes_written;
					p_buffer+=bytes_written;
				}	
			}
			break;
		}

        case ESYMBOLTYPE_NONE:
			// Empty array
			break;
						
        default:
            Dbg_MsgAssert(0,("Array type of '%s' is not supported in WriteToBuffer",GetTypeName(type)));
            break;
    }
	
	// Return how many bytes got written.
	return bufferSize-bytes_left;
}

// Calculates the size of buffer required if wanting to do a call to WriteToBuffer on the passed array.
uint32 CalculateBufferSize(CArray *p_array)
{
	// Need at least 3 bytes, one for the type, 2 for the size.
	uint32 space_required=3;
	uint32 i;
	
	uint32 size=p_array->GetSize();
	// Easy to change WriteToBuffer to support 4 byte sizes, but keeping as 2 for the moment for
	// backwards compatibility.
	Dbg_MsgAssert(size<0x10000,("Size of array too big, currently only 2 bytes used to store size in WriteToBuffer ..."));
	ESymbolType type=p_array->GetType();
	
	switch (type)
	{
        case ESYMBOLTYPE_INTEGER:
        case ESYMBOLTYPE_FLOAT:
        case ESYMBOLTYPE_NAME:
			space_required+=size*4;
			break;
			
        case ESYMBOLTYPE_STRING:
			for (i=0; i<size; ++i)
			{
				const char *p_string=p_array->GetString(i);
				Dbg_MsgAssert(p_string,("NULL GetString() for element %d when attempting to call CalculateBufferSize",i));			
				space_required+=strlen(p_string)+1;
			}
			break;

        case ESYMBOLTYPE_LOCALSTRING:
			for (i=0; i<size; ++i)
			{
				const char *p_string=p_array->GetLocalString(i);
				Dbg_MsgAssert(p_string,("NULL GetLocalString() for element %d when attempting to call CalculateBufferSize",i));			
				space_required+=strlen(p_string)+1;
			}
			break;
			
        case ESYMBOLTYPE_PAIR:
			#ifdef __NOPT_ASSERT__
			for (i=0; i<size; ++i)
			{
				CPair *p_pair=p_array->GetPair(i);
				Dbg_MsgAssert(p_pair,("NULL GetPair() for element %d when attempting to call CalculateBufferSize",i));
			}	
			#endif
			space_required+=size*8;
			break;
			
        case ESYMBOLTYPE_VECTOR:
			#ifdef __NOPT_ASSERT__
			for (i=0; i<size; ++i)
			{
				CVector *p_vector=p_array->GetVector(i);
				Dbg_MsgAssert(p_vector,("NULL GetVector() for element %d when attempting to call CalculateBufferSize",i));
			}	
			#endif
			space_required+=size*12;
			break;
		
        case ESYMBOLTYPE_STRUCTURE:
			for (i=0; i<size; ++i)
			{
				CStruct *p_structure=p_array->GetStructure(i);
				Dbg_MsgAssert(p_structure,("NULL GetStructure() for element %d when attempting to call CalculateBufferSize",i));
				space_required+=CalculateBufferSize(p_structure);
			}
			break;
		
        case ESYMBOLTYPE_ARRAY:
			for (i=0; i<size; ++i)
			{
				CArray *p_array_element=p_array->GetArray(i);
				Dbg_MsgAssert(p_array_element,("NULL GetArray() for element %d when attempting to call CalculateBufferSize",i));
				space_required+=CalculateBufferSize(p_array_element);
			}
			break;

        case ESYMBOLTYPE_NONE:
			// Empty array
			break;
						
        default:
            Dbg_MsgAssert(0,("Array type of '%s' is not supported in CalculateBufferSize",GetTypeName(type)));
            break;
    }
	
	return space_required;
}

// Sets the array's contents using the passed buffer, which was generated
// by WriteToBuffer.
// If the array contained anything to begin with, it will get cleared.	

// Returns a pointer to after the read array data.
uint8 *ReadFromBuffer(CArray *p_array, uint8 *p_buffer)
{
	Dbg_MsgAssert(p_array,("NULL p_array sent to ReadFromBuffer"));
	Dbg_MsgAssert(p_buffer,("NULL p_buffer sent to ReadFromBuffer"));

	// First clear anything currently in the array.
	CleanUpArray(p_array);

	// Get the type and size
	ESymbolType type=(ESymbolType)*p_buffer++;
	uint32 size=*p_buffer++;
	size+=*p_buffer++<<8; // 2 byte size.

	// Make sure we're using the script heap.
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().ScriptHeap());
	p_array->SetSizeAndType(size,type);
	Mem::Manager::sHandle().PopContext();
	
    switch (type)
    {
		case ESYMBOLTYPE_INTEGER:
		{
			for (uint32 i=0; i<size; ++i)
			{
				p_array->SetInteger(i,Read4Bytes(p_buffer).mInt);
				p_buffer+=4;
			}
			break;
		}		
		case ESYMBOLTYPE_FLOAT:
		{
			for (uint32 i=0; i<size; ++i)
			{
				p_array->SetFloat(i,Read4Bytes(p_buffer).mFloat);
				p_buffer+=4;
			}
			break;
		}		
		case ESYMBOLTYPE_NAME:
		{
			for (uint32 i=0; i<size; ++i)
			{
				p_array->SetChecksum(i,Read4Bytes(p_buffer).mChecksum);
				p_buffer+=4;
			}
			break;
		}		
		case ESYMBOLTYPE_STRING:
		{
			for (uint32 i=0; i<size; ++i)
			{
				p_array->SetString(i,CreateString((const char*)p_buffer));
				p_buffer+=strlen((char*)p_buffer)+1;
			}
			break;
		}		
		case ESYMBOLTYPE_LOCALSTRING:
		{
			for (uint32 i=0; i<size; ++i)
			{
				p_array->SetLocalString(i,CreateString((const char*)p_buffer));
				p_buffer+=strlen((char*)p_buffer)+1;
			}
			break;
		}		
		case ESYMBOLTYPE_PAIR:
		{
			for (uint32 i=0; i<size; ++i)
			{
				CPair *p_pair=new CPair;
				p_pair->mX=Read4Bytes(p_buffer).mFloat;
				p_buffer+=4;
				p_pair->mY=Read4Bytes(p_buffer).mFloat;
				p_buffer+=4;

				p_array->SetPair(i,p_pair);
			}		
			break;
		}	
		case ESYMBOLTYPE_VECTOR:
		{
			for (uint32 i=0; i<size; ++i)
			{
				CVector *p_vector=new CVector;
				p_vector->mX=Read4Bytes(p_buffer).mFloat;
				p_buffer+=4;
				p_vector->mY=Read4Bytes(p_buffer).mFloat;
				p_buffer+=4;
				p_vector->mZ=Read4Bytes(p_buffer).mFloat;
				p_buffer+=4;

				p_array->SetVector(i,p_vector);
			}		
			break;
		}	
		case ESYMBOLTYPE_STRUCTURE:
		{
			for (uint32 i=0; i<size; ++i)
			{
				CStruct *p_structure=new CStruct;
				p_buffer=ReadFromBuffer(p_structure,p_buffer);
				p_array->SetStructure(i,p_structure);
			}			
			break;
		}	
		case ESYMBOLTYPE_ARRAY:
		{
			for (uint32 i=0; i<size; ++i)
			{
				CArray *p_new_array=new CArray;
				p_buffer=ReadFromBuffer(p_new_array,p_buffer);
				p_array->SetArray(i,p_new_array);
			}			
			break;
		}	
		case ESYMBOLTYPE_NONE:
			// Empty array
			break;
		default:
			Dbg_MsgAssert(0,("Unsupported type of '%s' encountered in ReadFromBuffer",GetTypeName(type)));
			break;
	}
	
	return p_buffer;
}


union UValue
{
	int mIntegerValue;
	float mFloatValue;
	char *mpString;
	char *mpLocalString;
	CPair *mpPair;
	CVector *mpVector;
	CStruct *mpStructure;
	CArray *mpArray;
	uint8 *mpScript;
	uint32 mChecksum;
	uint32 mUnion;
};

// This will load type and value from the passed component, resolving it if it refers to a global.
static void Resolve(const CComponent& comp, ESymbolType &type, UValue &value)
{
	value.mUnion=comp.mUnion;
	type=(ESymbolType)comp.mType;
	
	if (type==ESYMBOLTYPE_NAME)
	{
		CSymbolTableEntry *p_entry=Resolve(value.mChecksum);
		if (p_entry)
		{
			value.mUnion=p_entry->mUnion;
			type=(ESymbolType)p_entry->mType;
		}	
	}
}

// The == operator is used when interpreting switch statements and expressions in script
// This function is here rather than in component.cpp to avoid cyclic dependencies.
bool CComponent::operator==( const CComponent& v ) const
{
	UValue value_a,value_b;
	ESymbolType type_a,type_b;
	Resolve(*this,type_a,value_a);
	Resolve(v,type_b,value_b);
	
	switch (type_a)
	{
		case ESYMBOLTYPE_INTEGER:
			if (type_b==ESYMBOLTYPE_FLOAT)
			{
				return ((float)value_a.mIntegerValue)==value_b.mFloatValue;
			}
			else if (type_b==ESYMBOLTYPE_INTEGER)
			{
				return value_a.mIntegerValue==value_b.mIntegerValue;
			}
			break;
		case ESYMBOLTYPE_FLOAT:
			if (type_b==ESYMBOLTYPE_INTEGER)
			{
				return value_a.mFloatValue==((float)value_b.mIntegerValue);
			}
			else if (type_b==ESYMBOLTYPE_FLOAT)
			{
				return value_a.mFloatValue==value_b.mFloatValue;
			}
			break;
		case ESYMBOLTYPE_STRING:
			if (type_b==ESYMBOLTYPE_STRING)
			{
				Dbg_MsgAssert(value_a.mpString,("NULL value_a.mpString ?"));
				Dbg_MsgAssert(value_b.mpString,("NULL value_b.mpString ?"));
				return strcmp(value_a.mpString,value_b.mpString)==0;
			}
			break;
		case ESYMBOLTYPE_LOCALSTRING:
			if (type_b==ESYMBOLTYPE_LOCALSTRING)
			{
				Dbg_MsgAssert(value_a.mpLocalString,("NULL value_a.mpLocalString ?"));
				Dbg_MsgAssert(value_b.mpLocalString,("NULL value_b.mpLocalString ?"));
				return strcmp(value_a.mpLocalString,value_b.mpLocalString)==0;
			}
			break;
		case ESYMBOLTYPE_NAME:
			if (type_b==ESYMBOLTYPE_NAME)
			{
				return value_a.mChecksum==value_b.mChecksum;
			}	
			break;
		case ESYMBOLTYPE_PAIR:
			if (type_b==ESYMBOLTYPE_PAIR)
			{
				Dbg_MsgAssert(value_a.mpPair,("NULL value_a.mpPair ?"));
				Dbg_MsgAssert(value_b.mpPair,("NULL value_b.mpPair ?"));
				return value_a.mpPair->mX==value_b.mpPair->mX && 
					   value_a.mpPair->mY==value_b.mpPair->mY;
			}
			break;
		case ESYMBOLTYPE_VECTOR:
			if (type_b==ESYMBOLTYPE_VECTOR)
			{
				Dbg_MsgAssert(value_a.mpVector,("NULL value_a.mpVector ?"));
				Dbg_MsgAssert(value_b.mpVector,("NULL value_b.mpVector ?"));
				return value_a.mpVector->mX==value_b.mpVector->mX && 
					   value_a.mpVector->mY==value_b.mpVector->mY &&
					   value_a.mpVector->mZ==value_b.mpVector->mZ;
			}
			break;
		case ESYMBOLTYPE_QSCRIPT:
			if (type_b==ESYMBOLTYPE_QSCRIPT)
			{
				return value_a.mpScript==value_b.mpScript;
			}
			break;
		case ESYMBOLTYPE_STRUCTURE:
			if (type_b==ESYMBOLTYPE_STRUCTURE)
			{
				Dbg_MsgAssert(value_a.mpStructure,("NULL value_a.mpStructure ?"));
				Dbg_MsgAssert(value_b.mpStructure,("NULL value_b.mpStructure ?"));
				
				// The structures definitely match if the pointers match, cos its the 
				// same bloomin' structure.
				if (value_a.mpStructure==value_b.mpStructure)
				{
					return true;
				}	
				
				// If the pointers do not match, the structures may still have contents that match.
				// Structure comparison's ain't supported yet though. Implement when needed.
				// Note: If this ever get's implemented, you'll need to deal with un-named name 
				// components that resolve to a global structure.
				#ifdef __NOPT_ASSERT__
				printf("CStruct comparisons are not supported yet ... implement when needed\n");
				#endif
			}
			break;
		case ESYMBOLTYPE_ARRAY:
			if (type_b==ESYMBOLTYPE_ARRAY)
			{
				Dbg_MsgAssert(value_a.mpArray,("NULL value_a.mpArray ?"));
				Dbg_MsgAssert(value_b.mpArray,("NULL value_b.mpArray ?"));
				
				// The arrays definitely match if the pointers match.
				if (value_a.mpArray==value_b.mpArray)
				{
					return true;
				}	
				
				// If the pointers do not match, the arrays may still have contents that match.
				return *value_a.mpArray==*value_b.mpArray;
			}
			break;
		case ESYMBOLTYPE_CFUNCTION:
			// TODO ...
			#ifdef __NOPT_ASSERT__
			printf("C-Function comparisons are not supported yet ... implement when needed\n");
			#endif
			break;
		case ESYMBOLTYPE_MEMBERFUNCTION:
			// TODO ...
			#ifdef __NOPT_ASSERT__
			printf("Member function comparisons are not supported yet ... implement when needed\n");
			#endif
			break;
		case ESYMBOLTYPE_NONE:
			break;
		default:
			#ifdef __NOPT_ASSERT__
			printf("'%s' with '%s' comparisons are not supported\n",GetTypeName(type_a),GetTypeName(type_b));			
			#endif
			break;
	}		
	
	return false;
}

bool CComponent::operator!=( const CComponent& v ) const
{
	return !(*this==v);
}

bool CComponent::operator<( const CComponent& v ) const
{
	UValue value_a,value_b;
	ESymbolType type_a,type_b;
	Resolve(*this,type_a,value_a);
	Resolve(v,type_b,value_b);
	
	switch (type_a)
	{
		case ESYMBOLTYPE_INTEGER:
			if (type_b==ESYMBOLTYPE_FLOAT)
			{
				return ((float)value_a.mIntegerValue) < value_b.mFloatValue;
			}
			else if (type_b==ESYMBOLTYPE_INTEGER)
			{
				return value_a.mIntegerValue < value_b.mIntegerValue;
			}
			break;
		case ESYMBOLTYPE_FLOAT:
			if (type_b==ESYMBOLTYPE_INTEGER)
			{
				return value_a.mFloatValue < ((float)value_b.mIntegerValue);
			}
			else if (type_b==ESYMBOLTYPE_FLOAT)
			{
				return value_a.mFloatValue < value_b.mFloatValue;
			}
			break;
		case ESYMBOLTYPE_STRING:
			if (type_b==ESYMBOLTYPE_STRING)
			{
				Dbg_MsgAssert(value_a.mpString,("NULL value_a.mpString ?"));
				Dbg_MsgAssert(value_b.mpString,("NULL value_b.mpString ?"));
				// Using string length rather than position in the alphabet, cos it's
				// probably more useful.
				return strlen(value_a.mpString) < strlen(value_b.mpString);
			}
			break;
		case ESYMBOLTYPE_LOCALSTRING:
			if (type_b==ESYMBOLTYPE_LOCALSTRING)
			{
				Dbg_MsgAssert(value_a.mpLocalString,("NULL value_a.mpLocalString ?"));
				Dbg_MsgAssert(value_b.mpLocalString,("NULL value_b.mpLocalString ?"));
				return strlen(value_a.mpLocalString) < strlen(value_b.mpLocalString);
			}
			break;
		default:
			#ifdef __NOPT_ASSERT__
			printf("'%s' with '%s' less-than comparisons are not supported\n",GetTypeName(type_a),GetTypeName(type_b));			
			#endif
			break;
	}		
	
	return false;
}

bool CComponent::operator>( const CComponent& v ) const
{
	UValue value_a,value_b;
	ESymbolType type_a,type_b;
	Resolve(*this,type_a,value_a);
	Resolve(v,type_b,value_b);
	
	switch (type_a)
	{
		case ESYMBOLTYPE_INTEGER:
			if (type_b==ESYMBOLTYPE_FLOAT)
			{
				return ((float)value_a.mIntegerValue) > value_b.mFloatValue;
			}
			else if (type_b==ESYMBOLTYPE_INTEGER)
			{
				return value_a.mIntegerValue > value_b.mIntegerValue;
			}
			break;
		case ESYMBOLTYPE_FLOAT:
			if (type_b==ESYMBOLTYPE_INTEGER)
			{
				return value_a.mFloatValue > ((float)value_b.mIntegerValue);
			}
			else if (type_b==ESYMBOLTYPE_FLOAT)
			{
				return value_a.mFloatValue > value_b.mFloatValue;
			}
			break;
		case ESYMBOLTYPE_STRING:
			if (type_b==ESYMBOLTYPE_STRING)
			{
				Dbg_MsgAssert(value_a.mpString,("NULL value_a.mpString ?"));
				Dbg_MsgAssert(value_b.mpString,("NULL value_b.mpString ?"));
				// Using string length rather than position in the alphabet, cos it's
				// probably more useful.
				return strlen(value_a.mpString) > strlen(value_b.mpString);
			}
			break;
		case ESYMBOLTYPE_LOCALSTRING:
			if (type_b==ESYMBOLTYPE_LOCALSTRING)
			{
				Dbg_MsgAssert(value_a.mpLocalString,("NULL value_a.mpLocalString ?"));
				Dbg_MsgAssert(value_b.mpLocalString,("NULL value_b.mpLocalString ?"));
				return strlen(value_a.mpLocalString) > strlen(value_b.mpLocalString);
			}
			break;
		default:
			#ifdef __NOPT_ASSERT__
			printf("'%s' with '%s' greater-than comparisons are not supported\n",GetTypeName(type_a),GetTypeName(type_b));			
			#endif
			break;
	}		
	
	return false;
}

bool CComponent::operator<=( const CComponent& v ) const
{
	if (*this<v)
	{
		return true;
	}
	if (*this==v)
	{
		return true;
	}
	return false;	
}

bool CComponent::operator>=( const CComponent& v ) const
{
	if (*this>v)
	{
		return true;
	}
	if (*this==v)
	{
		return true;
	}
	return false;	
}

// This is used in eval.cpp, when evaluating foo[3] say.
// Copies the array element indicated by index into the passed component.
// The type of p_comp may be ESYMBOLTYPE_NONE if the type is not supported yet by not being in
// the switch statement.
void CopyArrayElementIntoComponent(CArray *p_array, uint32 index, CComponent *p_comp)
{
	Dbg_MsgAssert(p_array,("NULL p_array"));
	Dbg_MsgAssert(index<p_array->GetSize(),("Array index %d out of range, array size is %d",index,p_array->GetSize()));
	Dbg_MsgAssert(p_comp,("NULL p_comp"));
	
	p_comp->mType=p_array->GetType();
	switch (p_comp->mType)
	{
	case ESYMBOLTYPE_INTEGER:
		p_comp->mIntegerValue=p_array->GetInteger(index);
		break;
	case ESYMBOLTYPE_FLOAT:
		p_comp->mFloatValue=p_array->GetFloat(index);
		break;
	case ESYMBOLTYPE_NAME:
		p_comp->mChecksum=p_array->GetChecksum(index);
		break;
	case ESYMBOLTYPE_STRUCTURE:
		p_comp->mpStructure=new CStruct;
		p_comp->mpStructure->AppendStructure(p_array->GetStructure(index));
		break;
	case ESYMBOLTYPE_ARRAY:
		p_comp->mpArray=new CArray;
		CopyArray(p_comp->mpArray,p_array->GetArray(index));
		break;
	case ESYMBOLTYPE_STRING:
		p_comp->mpString=CreateString(p_array->GetString(index));
		break;
	case ESYMBOLTYPE_VECTOR:
	{
		CVector *p_source_vector=p_array->GetVector(index);
		p_comp->mpVector=new CVector;
		p_comp->mpVector->mX=p_source_vector->mX;
		p_comp->mpVector->mY=p_source_vector->mY;
		p_comp->mpVector->mZ=p_source_vector->mZ;
		break;
	}	
	case ESYMBOLTYPE_PAIR:
	{
		CPair *p_source_pair=p_array->GetPair(index);
		p_comp->mpPair=new CPair;
		p_comp->mpPair->mX=p_source_pair->mX;
		p_comp->mpPair->mY=p_source_pair->mY;
		break;
	}	
	default:
		Dbg_MsgAssert(0,("The [] operator is not yet supported when the array element has  type '%s'",GetTypeName(p_comp->mType)));
		p_comp->mType=ESYMBOLTYPE_NONE;
		p_comp->mUnion=0;
		break;
	}	
}

// This is used in parse.cpp, and cfuncs.cpp in sFormatText
void ResolveNameComponent(CComponent *p_comp)
{
	Dbg_MsgAssert(p_comp,("NULL p_comp"));

	if (p_comp->mType!=ESYMBOLTYPE_NAME)
	{
		return;
	}
		
	CSymbolTableEntry *p_entry=Resolve(p_comp->mChecksum);
	if (!p_entry)
	{
		return;
	}
		
	switch (p_entry->mType)
	{
	case ESYMBOLTYPE_FLOAT:
		p_comp->mType=ESYMBOLTYPE_FLOAT;
		p_comp->mFloatValue=p_entry->mFloatValue;
		break;
	case ESYMBOLTYPE_INTEGER:
		p_comp->mType=ESYMBOLTYPE_INTEGER;
		p_comp->mIntegerValue=p_entry->mIntegerValue;
		break;
	case ESYMBOLTYPE_VECTOR:
		p_comp->mType=ESYMBOLTYPE_VECTOR;
		p_comp->mpVector=p_entry->mpVector;
		break;
	case ESYMBOLTYPE_PAIR:
		p_comp->mType=ESYMBOLTYPE_PAIR;
		p_comp->mpPair=p_entry->mpPair;
		break;
	case ESYMBOLTYPE_STRING:
		p_comp->mType=ESYMBOLTYPE_STRING;
		p_comp->mpString=p_entry->mpString;
		break;
	case ESYMBOLTYPE_LOCALSTRING:
		p_comp->mType=ESYMBOLTYPE_LOCALSTRING;
		p_comp->mpLocalString=p_entry->mpLocalString;
		break;
	case ESYMBOLTYPE_STRUCTURE:
		p_comp->mType=ESYMBOLTYPE_STRUCTURE;
		p_comp->mpStructure=p_entry->mpStructure;
		break;
	case ESYMBOLTYPE_ARRAY:
		p_comp->mType=ESYMBOLTYPE_ARRAY;
		p_comp->mpArray=p_entry->mpArray;
		break;
	default:
		break;
	}
}

} // namespace Script

