#include <gel/scripting/eval.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/array.h>
#include <gel/scripting/vecpair.h>
#include <gel/scripting/string.h>
#include <gel/scripting/utils.h>
#include <gel/scripting/parse.h>

namespace Script
{

// This is a static rather than a local var of execute_operation() to prevent the 
// constructor being called every time execute_operation is called.
// Saves a tiny bit of time.
CComponent spResult;

static int sPrecedence[]=
{
	-1, // ESCRIPTTOKEN_ENDOFFILE,			// 0
	-1, // ESCRIPTTOKEN_ENDOFLINE,			// 1
	-1, // ESCRIPTTOKEN_ENDOFLINENUMBER,   // 2
	-1, // ESCRIPTTOKEN_STARTSTRUCT,       // 3
	-1, // ESCRIPTTOKEN_ENDSTRUCT,         // 4
	100, // ESCRIPTTOKEN_STARTARRAY,        // 5
	-1, // ESCRIPTTOKEN_ENDARRAY,          // 6
	76, // ESCRIPTTOKEN_EQUALS,            // 7
	100, // ESCRIPTTOKEN_DOT,               // 8
	-1, // ESCRIPTTOKEN_COMMA,             // 9
	98, // ESCRIPTTOKEN_MINUS,             // 10
	98, // ESCRIPTTOKEN_ADD,               // 11
	99, // ESCRIPTTOKEN_DIVIDE,            // 12
	100, // ESCRIPTTOKEN_MULTIPLY,          // 13
	-1, // ESCRIPTTOKEN_OPENPARENTH,       // 14
	-1, // ESCRIPTTOKEN_CLOSEPARENTH,      // 15
	-1, // ESCRIPTTOKEN_DEBUGINFO,			// 16
	-1, // ESCRIPTTOKEN_SAMEAS,			// 17
	80, // ESCRIPTTOKEN_LESSTHAN,			// 18
	79, // ESCRIPTTOKEN_LESSTHANEQUAL,     // 19
	78, // ESCRIPTTOKEN_GREATERTHAN,       // 20
	77, // ESCRIPTTOKEN_GREATERTHANEQUAL,  // 21
	-1, // ESCRIPTTOKEN_NAME,				// 22
	-1, // ESCRIPTTOKEN_INTEGER,			// 23
	-1, // ESCRIPTTOKEN_HEXINTEGER,        // 24
    -1, // ESCRIPTTOKEN_ENUM,              // 25
	-1, // ESCRIPTTOKEN_FLOAT,             // 26
	-1, // ESCRIPTTOKEN_STRING,            // 27
	-1, // ESCRIPTTOKEN_LOCALSTRING,       // 28
	-1, // ESCRIPTTOKEN_ARRAY,             // 29
	-1, // ESCRIPTTOKEN_VECTOR,            // 30
	-1, // ESCRIPTTOKEN_PAIR,				// 31
	-1, // ESCRIPTTOKEN_KEYWORD_BEGIN,		// 32
	-1, // ESCRIPTTOKEN_KEYWORD_REPEAT,    // 33
	-1, // ESCRIPTTOKEN_KEYWORD_BREAK,     // 34
	-1, // ESCRIPTTOKEN_KEYWORD_SCRIPT,    // 35
	-1, // ESCRIPTTOKEN_KEYWORD_ENDSCRIPT, // 36
	-1, // ESCRIPTTOKEN_KEYWORD_IF,        // 37
	-1, // ESCRIPTTOKEN_KEYWORD_ELSE,      // 38
	-1, // ESCRIPTTOKEN_KEYWORD_ELSEIF,    // 39
	-1, // ESCRIPTTOKEN_KEYWORD_ENDIF,		// 40
	-1, // ESCRIPTTOKEN_KEYWORD_RETURN,	// 41
    -1, // ESCRIPTTOKEN_UNDEFINED,			// 42
	-1, // ESCRIPTTOKEN_CHECKSUM_NAME,		// 43
	-1, // ESCRIPTTOKEN_KEYWORD_ALLARGS,	// 44
	-1, // ESCRIPTTOKEN_ARG,				// 45
	-1, // ESCRIPTTOKEN_JUMP,				// 46
	-1, // ESCRIPTTOKEN_KEYWORD_RANDOM,    // 47
	-1, // ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE,	// 48
	-1, // ESCRIPTTOKEN_AT,				// 49
	58, // ESCRIPTTOKEN_OR,				// 50
	60, // ESCRIPTTOKEN_AND,				// 51
	59, // ESCRIPTTOKEN_XOR,				// 52
	90, // ESCRIPTTOKEN_SHIFT_LEFT,		// 53
	89, // ESCRIPTTOKEN_SHIFT_RIGHT,		// 54
	-1, // ESCRIPTTOKEN_KEYWORD_RANDOM2,		// 55
	-1, // ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE2, // 56
	-1, // ESCRIPTTOKEN_KEYWORD_NOT,			// 57
	-1, // ESCRIPTTOKEN_KEYWORD_AND,			// 58
	-1, // ESCRIPTTOKEN_KEYWORD_OR,            // 59
	-1, // ESCRIPTTOKEN_KEYWORD_SWITCH,       	// 60
	-1, // ESCRIPTTOKEN_KEYWORD_ENDSWITCH,   	// 61
	-1, // ESCRIPTTOKEN_KEYWORD_CASE,          // 62
	-1, // ESCRIPTTOKEN_KEYWORD_DEFAULT,		// 63
	-1,	// ESCRIPTTOKEN_KEYWORD_RANDOM_NO_REPEAT,	// 64
	-1,	// ESCRIPTTOKEN_KEYWORD_RANDOM_PERMUTE,	// 65
	-1, // ESCRIPTTOKEN_COLON,		// 66
	-1, // ESCRIPTTOKEN_RUNTIME_CFUNCTION,	// 67
	-1, // ESCRIPTTOKEN_RUNTIME_MEMBERFUNCTION, // 68
};

static bool sSameOrLowerPrecedence(EScriptToken a, EScriptToken b)
{
	//printf("Precedence of %s=%d, %s=%d\n",GetTokenName(a),sPrecedence[a],GetTokenName(b),sPrecedence[b]);
	if (a==b)
	{
		return true;
	}
	
	return sPrecedence[a]<=sPrecedence[b];
}

CExpressionEvaluator::CExpressionEvaluator()
{
	Clear();
	EnableErrorChecking();
}

CExpressionEvaluator::~CExpressionEvaluator()
{
	Clear();
}

void CExpressionEvaluator::Clear()
{
	int i;

	for (i=0; i<VALUE_STACK_SIZE; ++i)
	{
		CleanUpComponent(&mp_value_stack[i]);
	}	
	m_value_stack_top=0;
	
	m_operator_stack_top=0;
	for (i=0; i<OPERATOR_STACK_SIZE; ++i)
	{
		mp_operator_stack[i].mOperator=NOP;
		mp_operator_stack[i].mParenthesesCount=0;
	}	
	m_got_operators=false;
		
	mp_error_string=NULL;
}

void CExpressionEvaluator::ClearIfNeeded()
{
	if (m_value_stack_top || 
		mp_value_stack[0].mType!=ESYMBOLTYPE_NONE ||
		m_operator_stack_top || mp_error_string)
	{
		Clear();
	}	
}

void CExpressionEvaluator::SetTokenPointer(uint8 *p_token)
{
	mp_token=p_token;
}

void CExpressionEvaluator::set_error(const char *p_error)
{
	// TODO: Make this support variable arguments so it works like printf.
	Clear();
	mp_error_string=p_error;
	
	// Sometimes we don't want an assert to go off when there is an error, such as
	// when scanning though the node scripts looking for EndGap commands.
	// (See the ScanNodeScripts function in nodearray.cpp)
	if (m_errors_enabled)
	{
		if (mp_token)
		{
			Dbg_MsgAssert(0,("Expression evaluator error, file %s, line %d: %s\n",Script::GetSourceFile(mp_token),Script::GetLineNumber(mp_token),p_error));
		}
		else
		{
			Dbg_MsgAssert(0,("Expression evaluator error: %s\n",p_error));
		}
	}	
}	

static int GetBool(CComponent *p_comp)
{
	if (p_comp->mType==ESYMBOLTYPE_INTEGER)
	{
		return p_comp->mIntegerValue;
	}
	else if (p_comp->mType==ESYMBOLTYPE_FLOAT)
	{
		return p_comp->mFloatValue?1:0;
	}
	else
	{
		#ifdef __NOPT_ASSERT__
		// So that the calling code can set an error message.
		// Not asserting at this point because there is no way of printing the line number of the error.
		return 2;
		#else
		return 0;
		#endif	
	}
}

void CExpressionEvaluator::execute_operation()
{
	if (m_value_stack_top<1)
	{
		set_error("Not enough values in stack to execute operation");
		return;
	}	
	if (mp_operator_stack[m_operator_stack_top].mParenthesesCount)
	{
		set_error("Non-zero parentheses count");
		return;
	}	
	
	// Apply the top operator to the top two values, and replace them with the new value.
	// Then remove the top operator.
	
	EScriptToken op=mp_operator_stack[m_operator_stack_top].mOperator;
	if (op==NOP)
	{
		set_error("Tried to execute with no operator");
		return;
	}

	CComponent *pA=&mp_value_stack[m_value_stack_top-1];
	CComponent *pB=&mp_value_stack[m_value_stack_top];
	
	spResult.mType=ESYMBOLTYPE_NONE;
	spResult.mUnion=0;

	//printf("Executing '%s'\n",GetTokenName(op));
	switch (op)
	{
	case ESCRIPTTOKEN_OR:
	{
		spResult.mType=ESYMBOLTYPE_INTEGER;
		int a=GetBool(pA);
		int b=GetBool(pB);
		#ifdef __NOPT_ASSERT__
		if (a==2 || b==2)
		{
			set_error("Bad data types for 'or' operator");
		}
		#endif
		spResult.mIntegerValue=a | b;
		break;
	}
	case ESCRIPTTOKEN_AND:
	{
		spResult.mType=ESYMBOLTYPE_INTEGER;
		int a=GetBool(pA);
		int b=GetBool(pB);
		#ifdef __NOPT_ASSERT__
		if (a==2 || b==2)
		{
			set_error("Bad data types for 'or' operator");
		}
		#endif
		spResult.mIntegerValue=a & b;
		break;
	}
	
	case ESCRIPTTOKEN_EQUALS:
		spResult.mType=ESYMBOLTYPE_INTEGER;
		spResult.mIntegerValue=*pA==*pB;
		break;
		
	case ESCRIPTTOKEN_LESSTHAN:
		spResult.mType=ESYMBOLTYPE_INTEGER;
		spResult.mIntegerValue=*pA<*pB;
		break;

	case ESCRIPTTOKEN_GREATERTHAN:
		spResult.mType=ESYMBOLTYPE_INTEGER;
		spResult.mIntegerValue=*pA>*pB;
		break;
	
	case ESCRIPTTOKEN_ADD:
		switch (pA->mType)
		{
		case ESYMBOLTYPE_INTEGER:
			if (pB->mType==ESYMBOLTYPE_INTEGER)
			{
				spResult.mType=ESYMBOLTYPE_INTEGER;
				spResult.mIntegerValue=pA->mIntegerValue+pB->mIntegerValue;
			}
			else if (pB->mType==ESYMBOLTYPE_FLOAT)
			{
				spResult.mType=ESYMBOLTYPE_FLOAT;
				spResult.mFloatValue=(float)pA->mIntegerValue+pB->mFloatValue;
			}
			else
			{
				set_error("Second arg cannot be added to an integer");
			}	
			break;
		case ESYMBOLTYPE_FLOAT:
			if (pB->mType==ESYMBOLTYPE_INTEGER)
			{
				spResult.mType=ESYMBOLTYPE_FLOAT;
				spResult.mFloatValue=pA->mFloatValue+(float)pB->mIntegerValue;
			}
			else if (pB->mType==ESYMBOLTYPE_FLOAT)
			{
				spResult.mType=ESYMBOLTYPE_FLOAT;
				spResult.mFloatValue=pA->mFloatValue+pB->mFloatValue;
			}
			else
			{
				set_error("Second arg cannot be added to a float");
			}	
			break;
		// Checksums may have integers added to them to give a new checksum.
		// This was added cos Steve needed to be able to give new unique id's
		// to some screen elements by adding an offset to a base checksum, 
		// using id=(achecksum+1) for example.
 		case ESYMBOLTYPE_NAME:
			if (pB->mType==ESYMBOLTYPE_INTEGER)
			{
				spResult.mType=ESYMBOLTYPE_NAME;
				spResult.mChecksum=pA->mChecksum+pB->mIntegerValue;
			}
			else
			{
				set_error("Second arg cannot be added to a checksum");
			}	
			break;
		case ESYMBOLTYPE_VECTOR:
			if (pB->mType==ESYMBOLTYPE_VECTOR)
			{
				spResult.mType=ESYMBOLTYPE_VECTOR;
				spResult.mpVector=new CVector;
				spResult.mpVector->mX=pA->mpVector->mX+pB->mpVector->mX;
				spResult.mpVector->mY=pA->mpVector->mY+pB->mpVector->mY;
				spResult.mpVector->mZ=pA->mpVector->mZ+pB->mpVector->mZ;
			}
			else
			{
				set_error("Second arg cannot be added to a vector");
			}	
			break;
		case ESYMBOLTYPE_PAIR:
			if (pB->mType==ESYMBOLTYPE_PAIR)
			{
				spResult.mType=ESYMBOLTYPE_PAIR;
				spResult.mpPair=new CPair;
				spResult.mpPair->mX=pA->mpPair->mX+pB->mpPair->mX;
				spResult.mpPair->mY=pA->mpPair->mY+pB->mpPair->mY;
			}
			else
			{
				set_error("Second arg cannot be added to a pair");
			}	
			break;
		case ESYMBOLTYPE_STRING:
			if (pB->mType==ESYMBOLTYPE_STRING)
			{
				if (strlen(pA->mpString)+strlen(pB->mpString)+1>EVALUATOR_STRING_BUF_SIZE)
				{
					set_error("Result of string addition too long");
				}
				else
				{
					spResult.mType=ESYMBOLTYPE_STRING;
					char p_buf[EVALUATOR_STRING_BUF_SIZE];
					strcpy(p_buf,pA->mpString);
					strcat(p_buf,pB->mpString);
					spResult.mpString=CreateString(p_buf);
				}	
			}
			else
			{
				set_error("Second arg cannot be added to a string");
			}	
			break;
		case ESYMBOLTYPE_LOCALSTRING:
			if (pB->mType==ESYMBOLTYPE_LOCALSTRING)
			{
				if (strlen(pA->mpLocalString)+strlen(pB->mpLocalString)+1>EVALUATOR_STRING_BUF_SIZE)
				{
					set_error("Result of local-string addition too long");
				}
				else
				{
					spResult.mType=ESYMBOLTYPE_LOCALSTRING;
					char p_buf[EVALUATOR_STRING_BUF_SIZE];
					strcpy(p_buf,pA->mpLocalString);
					strcat(p_buf,pB->mpLocalString);
					spResult.mpLocalString=CreateString(p_buf);
				}	
			}
			else
			{
				set_error("Second arg cannot be added to a local-string");
			}	
			break;
		case ESYMBOLTYPE_STRUCTURE:
		{
			if (pB->mType!=ESYMBOLTYPE_STRUCTURE)
			{
				set_error("Structure add operator requires 2nd arg to be a structure");
				break;
			}

			spResult.mType=ESYMBOLTYPE_STRUCTURE;
			spResult.mpStructure=new CStruct;
			spResult.mpStructure->AppendStructure(pA->mpStructure);
			spResult.mpStructure->AppendStructure(pB->mpStructure);
			break;
		}	
		default:
			set_error("Addition not supported for this type");
			break;
		}	
		break;
	case ESCRIPTTOKEN_MINUS:
		switch (pA->mType)
		{
		case ESYMBOLTYPE_INTEGER:
			if (pB->mType==ESYMBOLTYPE_INTEGER)
			{
				spResult.mType=ESYMBOLTYPE_INTEGER;
				spResult.mIntegerValue=pA->mIntegerValue-pB->mIntegerValue;
			}
			else if (pB->mType==ESYMBOLTYPE_FLOAT)
			{
				spResult.mType=ESYMBOLTYPE_FLOAT;
				spResult.mFloatValue=(float)pA->mIntegerValue-pB->mFloatValue;
			}
			else
			{
				set_error("Second arg cannot be subtracted from an integer");
			}	
			break;
		case ESYMBOLTYPE_FLOAT:
			if (pB->mType==ESYMBOLTYPE_INTEGER)
			{
				spResult.mType=ESYMBOLTYPE_FLOAT;
				spResult.mFloatValue=pA->mFloatValue-(float)pB->mIntegerValue;
			}
			else if (pB->mType==ESYMBOLTYPE_FLOAT)
			{
				spResult.mType=ESYMBOLTYPE_FLOAT;
				spResult.mFloatValue=pA->mFloatValue-pB->mFloatValue;
			}
			else
			{
				set_error("Second arg cannot be subtracted from a float");
			}	
			break;
		case ESYMBOLTYPE_VECTOR:
			if (pB->mType==ESYMBOLTYPE_VECTOR)
			{
				spResult.mType=ESYMBOLTYPE_VECTOR;
				spResult.mpVector=new CVector;
				spResult.mpVector->mX=pA->mpVector->mX-pB->mpVector->mX;
				spResult.mpVector->mY=pA->mpVector->mY-pB->mpVector->mY;
				spResult.mpVector->mZ=pA->mpVector->mZ-pB->mpVector->mZ;
			}
			else
			{
				set_error("Second arg cannot be subtracted from a vector");
			}	
			break;
		case ESYMBOLTYPE_PAIR:
			if (pB->mType==ESYMBOLTYPE_PAIR)
			{
				spResult.mType=ESYMBOLTYPE_PAIR;
				spResult.mpPair=new CPair;
				spResult.mpPair->mX=pA->mpPair->mX-pB->mpPair->mX;
				spResult.mpPair->mY=pA->mpPair->mY-pB->mpPair->mY;
			}
			else
			{
				set_error("Second arg cannot be subtracted from a pair");
			}	
			break;
		case ESYMBOLTYPE_STRUCTURE:
		{
			// If the LHS is a structure, then the subtract operator is defined
			// so as to allow parameters to be removed from the structure,
			// cos that's quite handy.
			
			// If the RHS is a single name, then all parameter with that name
			// will be removed, including flags.
			// If the RHS is an array of names then each of those parameters will
			// be removed.
			// If the RHS is a structure, then all the parameters in the LHS which have
			// matching name AND type to something in the RHS will be removed, but if the RHS
			// contains unnamed-names then all parameters with that name will be removed.
			// So {x=3 x="blaa"}-{x=9} gives {x="blaa"}
			// But {x=3 x="blaa"}-{x} gives {}
			
			if (pB->mType==ESYMBOLTYPE_NAME)
			{
				spResult.mType=ESYMBOLTYPE_STRUCTURE;
				spResult.mpStructure=new CStruct;
				spResult.mpStructure->AppendStructure(pA->mpStructure);
				spResult.mpStructure->RemoveComponent(pB->mChecksum);
				spResult.mpStructure->RemoveFlag(pB->mChecksum);
			}
			else if (pB->mType==ESYMBOLTYPE_ARRAY)
			{
				Script::CArray *p_array=pB->mpArray;
				Dbg_MsgAssert(p_array,("NULL p_array"));
				
				if (!(p_array->GetType() == ESYMBOLTYPE_NAME || 
					  p_array->GetType() == ESYMBOLTYPE_NONE)) // Allowing NONE so that empty arrays work.
				{
					set_error("Subtracting an array from a structure requires the array to be an array of names");
				}
				else
				{
					spResult.mType=ESYMBOLTYPE_STRUCTURE;
					spResult.mpStructure=new CStruct;
					spResult.mpStructure->AppendStructure(pA->mpStructure);
					
					for (uint32 i=0; i<p_array->GetSize(); ++i)
					{
						spResult.mpStructure->RemoveComponent(p_array->GetChecksum(i));
						spResult.mpStructure->RemoveFlag(p_array->GetChecksum(i));
					}
				}
			}
			else if (pB->mType==ESYMBOLTYPE_STRUCTURE)
			{
				spResult.mType=ESYMBOLTYPE_STRUCTURE;
				spResult.mpStructure=new CStruct;
				spResult.mpStructure->AppendStructure(pA->mpStructure);
				
				Script::CStruct *p_struct=pB->mpStructure;
				Dbg_MsgAssert(p_struct,("NULL p_struct"));
				Script::CComponent *p_comp=p_struct->GetNextComponent();
				while (p_comp)
				{
					if (p_comp->mNameChecksum)
					{
						// If the component is named, remove all components of the same name and type.
						spResult.mpStructure->RemoveComponentWithType(p_comp->mNameChecksum,p_comp->mType);
					}
					else if (p_comp->mType==ESYMBOLTYPE_NAME)
					{
						// If it's an unnamed-name (ie a flag) remove all components with that name,
						// including flags.
						spResult.mpStructure->RemoveComponent(p_comp->mChecksum);
						spResult.mpStructure->RemoveFlag(p_comp->mChecksum);
					}					
					p_comp=p_struct->GetNextComponent(p_comp);
				}	
			}
			else
			{
				set_error("Structure minus operator requires 2nd arg to be a name, an array of names, or a structure");
			}			
			break;
		}	
		default:
			set_error("Subtraction not supported for this type");
			break;
		}	
		break;
	case ESCRIPTTOKEN_MULTIPLY:
		switch (pA->mType)
		{
		case ESYMBOLTYPE_INTEGER:
			switch (pB->mType)
			{
			case ESYMBOLTYPE_INTEGER:
				spResult.mType=ESYMBOLTYPE_INTEGER;
				spResult.mIntegerValue=pA->mIntegerValue*pB->mIntegerValue;
				break;
			case ESYMBOLTYPE_FLOAT:
				spResult.mType=ESYMBOLTYPE_FLOAT;
				spResult.mFloatValue=(float)pA->mIntegerValue*pB->mFloatValue;
				break;
			case ESYMBOLTYPE_VECTOR:
				spResult.mType=ESYMBOLTYPE_VECTOR;
				spResult.mpVector=new CVector;
				spResult.mpVector->mX=(float)pA->mIntegerValue*pB->mpVector->mX;
				spResult.mpVector->mY=(float)pA->mIntegerValue*pB->mpVector->mY;
				spResult.mpVector->mZ=(float)pA->mIntegerValue*pB->mpVector->mZ;
				break;
			case ESYMBOLTYPE_PAIR:
				spResult.mType=ESYMBOLTYPE_PAIR;
				spResult.mpPair=new CPair;
				spResult.mpPair->mX=(float)pA->mIntegerValue*pB->mpPair->mX;
				spResult.mpPair->mY=(float)pA->mIntegerValue*pB->mpPair->mY;
				break;
			default:
				set_error("Second arg cannot be multiplied by an integer");
				break;
			}	
			break;
		case ESYMBOLTYPE_FLOAT:
			switch (pB->mType)
			{
			case ESYMBOLTYPE_INTEGER:
				spResult.mType=ESYMBOLTYPE_FLOAT;
				spResult.mFloatValue=pA->mFloatValue*(float)pB->mIntegerValue;
				break;
			case ESYMBOLTYPE_FLOAT:
				spResult.mType=ESYMBOLTYPE_FLOAT;
				spResult.mFloatValue=pA->mFloatValue*pB->mFloatValue;
				break;
			case ESYMBOLTYPE_VECTOR:
				spResult.mType=ESYMBOLTYPE_VECTOR;
				spResult.mpVector=new CVector;
				spResult.mpVector->mX=pA->mFloatValue*pB->mpVector->mX;
				spResult.mpVector->mY=pA->mFloatValue*pB->mpVector->mY;
				spResult.mpVector->mZ=pA->mFloatValue*pB->mpVector->mZ;
				break;
			case ESYMBOLTYPE_PAIR:
				spResult.mType=ESYMBOLTYPE_PAIR;
				spResult.mpPair=new CPair;
				spResult.mpPair->mX=pA->mFloatValue*pB->mpPair->mX;
				spResult.mpPair->mY=pA->mFloatValue*pB->mpPair->mY;
				break;
			default:
				set_error("Second arg cannot be multiplied by a float");
			}	
			break;
		case ESYMBOLTYPE_VECTOR:
			switch (pB->mType)
			{
			case ESYMBOLTYPE_VECTOR:
				// Take multiplication of two vectors to mean the cross-product.
				spResult.mType=ESYMBOLTYPE_VECTOR;
				spResult.mpVector=new CVector;
				spResult.mpVector->mX=pA->mpVector->mY*pB->mpVector->mZ-pA->mpVector->mZ*pB->mpVector->mY;
				spResult.mpVector->mY=pA->mpVector->mZ*pB->mpVector->mX-pA->mpVector->mX*pB->mpVector->mZ;
				spResult.mpVector->mZ=pA->mpVector->mX*pB->mpVector->mY-pA->mpVector->mY*pB->mpVector->mX;
				break;
			case ESYMBOLTYPE_FLOAT:
				spResult.mType=ESYMBOLTYPE_VECTOR;
				spResult.mpVector=new CVector;
				spResult.mpVector->mX=pA->mpVector->mX*pB->mFloatValue;
				spResult.mpVector->mY=pA->mpVector->mY*pB->mFloatValue;
				spResult.mpVector->mZ=pA->mpVector->mZ*pB->mFloatValue;
				break;
			case ESYMBOLTYPE_INTEGER:
				spResult.mType=ESYMBOLTYPE_VECTOR;
				spResult.mpVector=new CVector;
				spResult.mpVector->mX=pA->mpVector->mX*(float)pB->mIntegerValue;
				spResult.mpVector->mY=pA->mpVector->mY*(float)pB->mIntegerValue;
				spResult.mpVector->mZ=pA->mpVector->mZ*(float)pB->mIntegerValue;
				break;
			default:
				set_error("Vector cannot be multiplied by second arg");
				break;
			}	
			break;
		case ESYMBOLTYPE_PAIR:
			switch (pB->mType)
			{
			case ESYMBOLTYPE_FLOAT:
				spResult.mType=ESYMBOLTYPE_PAIR;
				spResult.mpPair=new CPair;
				spResult.mpPair->mX=pA->mpPair->mX*pB->mFloatValue;
				spResult.mpPair->mY=pA->mpPair->mY*pB->mFloatValue;
				break;
			case ESYMBOLTYPE_INTEGER:
				spResult.mType=ESYMBOLTYPE_PAIR;
				spResult.mpPair=new CPair;
				spResult.mpPair->mX=pA->mpPair->mX*(float)pB->mIntegerValue;
				spResult.mpPair->mY=pA->mpPair->mY*(float)pB->mIntegerValue;
				break;
			default:
				set_error("Pair cannot be multiplied by second arg");
				break;
			}	
			break;
		default:
			set_error("Multiplication not supported for this type");
			break;
		}	
		break;
	case ESCRIPTTOKEN_DIVIDE:
		if (pB->mType==ESYMBOLTYPE_INTEGER)
		{
			if (pB->mIntegerValue==0)
			{
				set_error("Integer division by zero");
				break;
			}
		}
		else if (pB->mType==ESYMBOLTYPE_FLOAT)
		{
			if (pB->mFloatValue==0.0f)
			{
				set_error("Float division by zero");
				break;
			}
		}
		
		switch (pA->mType)
		{
		case ESYMBOLTYPE_INTEGER:
			if (pB->mType==ESYMBOLTYPE_INTEGER)
			{
				spResult.mType=ESYMBOLTYPE_INTEGER;
				spResult.mIntegerValue=pA->mIntegerValue/pB->mIntegerValue;
			}
			else if (pB->mType==ESYMBOLTYPE_FLOAT)
			{
				spResult.mType=ESYMBOLTYPE_FLOAT;
				spResult.mFloatValue=(float)pA->mIntegerValue/pB->mFloatValue;
			}
			else
			{
				set_error("Integer cannot be divided by second arg");
			}	
			break;
		case ESYMBOLTYPE_FLOAT:
			if (pB->mType==ESYMBOLTYPE_INTEGER)
			{
				spResult.mType=ESYMBOLTYPE_FLOAT;
				spResult.mFloatValue=pA->mFloatValue/(float)pB->mIntegerValue;
			}
			else if (pB->mType==ESYMBOLTYPE_FLOAT)
			{
				spResult.mType=ESYMBOLTYPE_FLOAT;
				spResult.mFloatValue=pA->mFloatValue/pB->mFloatValue;
			}
			else
			{
				set_error("Float cannot be divided by second arg");
			}	
			break;
		case ESYMBOLTYPE_VECTOR:
			if (pB->mType==ESYMBOLTYPE_INTEGER)
			{
				spResult.mType=ESYMBOLTYPE_VECTOR;
				spResult.mpVector=new CVector;
				spResult.mpVector->mX=pA->mpVector->mX/(float)pB->mIntegerValue;
				spResult.mpVector->mY=pA->mpVector->mY/(float)pB->mIntegerValue;
				spResult.mpVector->mZ=pA->mpVector->mZ/(float)pB->mIntegerValue;
			}
			else if (pB->mType==ESYMBOLTYPE_FLOAT)
			{
				spResult.mType=ESYMBOLTYPE_VECTOR;
				spResult.mpVector=new CVector;
				spResult.mpVector->mX=pA->mpVector->mX/pB->mFloatValue;
				spResult.mpVector->mY=pA->mpVector->mY/pB->mFloatValue;
				spResult.mpVector->mZ=pA->mpVector->mZ/pB->mFloatValue;
			}
			else
			{
				set_error("Vector cannot be divided by second arg");
			}	
			break;
		case ESYMBOLTYPE_PAIR:
			if (pB->mType==ESYMBOLTYPE_INTEGER)
			{
				spResult.mType=ESYMBOLTYPE_PAIR;
				spResult.mpPair=new CPair;
				spResult.mpPair->mX=pA->mpPair->mX/(float)pB->mIntegerValue;
				spResult.mpPair->mY=pA->mpPair->mY/(float)pB->mIntegerValue;
			}
			else if (pB->mType==ESYMBOLTYPE_FLOAT)
			{
				spResult.mType=ESYMBOLTYPE_PAIR;
				spResult.mpPair=new CPair;
				spResult.mpPair->mX=pA->mpPair->mX/pB->mFloatValue;
				spResult.mpPair->mY=pA->mpPair->mY/pB->mFloatValue;
			}
			else
			{
				set_error("Pair cannot be divided by second arg");
			}	
			break;
		default:
			set_error("Division not supported for this type");
			break;
		}	
		break;
	case ESCRIPTTOKEN_DOT:
		switch (pA->mType)
		{
		case ESYMBOLTYPE_VECTOR:
			if (pB->mType!=ESYMBOLTYPE_VECTOR)
			{
				set_error("Vector can only be dot producted with another vector");
				break;
			}
			spResult.mType=ESYMBOLTYPE_FLOAT;
			spResult.mFloatValue=pA->mpVector->mX*pB->mpVector->mX+
								 pA->mpVector->mY*pB->mpVector->mY+
								 pA->mpVector->mZ*pB->mpVector->mZ;
			break;	
		case ESYMBOLTYPE_PAIR:
			if (pB->mType!=ESYMBOLTYPE_PAIR)
			{
				set_error("Pair can only be dot producted with another pair");
				break;
			}
			spResult.mType=ESYMBOLTYPE_FLOAT;
			spResult.mFloatValue=pA->mpPair->mX*pB->mpPair->mX+
								 pA->mpPair->mY*pB->mpPair->mY;
			break;	
		case ESYMBOLTYPE_STRUCTURE:
		{
			if (pB->mType!=ESYMBOLTYPE_NAME)
			{
				set_error("Structure dot operator requires 2nd arg to be a name");
				break;
			}
			Dbg_MsgAssert(pA->mpStructure,("NULL pA->mpStructure"));
			CComponent *p_found=pA->mpStructure->FindNamedComponentRecurse(pB->mChecksum);
			if (p_found)
			{
				CopyComponent(&spResult,p_found);
			}	
			break;
		}	
		default:
			set_error("Dot product not supported for this type");
			break;
		}
		break;
	case ESCRIPTTOKEN_STARTARRAY:
	{
		if (pA->mType!=ESYMBOLTYPE_ARRAY)
		{
			set_error("Array element operator not supported for this type");
			break;
		}
		Dbg_MsgAssert(pA->mpArray,("NULL pA->mpArray"));
		
		if (pB->mType!=ESYMBOLTYPE_INTEGER)
		{
			set_error("[] index must be an integer");
			break;
		}
		uint32 index=pB->mIntegerValue;
		
		if (index>=pA->mpArray->GetSize())
		{
			set_error("[] index out of range");
			break;
		}
		
		CopyArrayElementIntoComponent(pA->mpArray,index,&spResult);
		if (spResult.mType==ESYMBOLTYPE_NONE)
		{
			set_error("Array type cannot be accessed using [] yet ...");
			break;
		}		
				
		break;
	}	
	default:
		set_error("Operator not supported");
		break;
	}
				
	CleanUpComponent(pA);
	CleanUpComponent(pB);
	pA->mType=spResult.mType;
	pA->mUnion=spResult.mUnion;
	spResult.mType=ESYMBOLTYPE_NONE;
	spResult.mUnion=0;
	
	--m_value_stack_top;
	
	mp_operator_stack[m_operator_stack_top].mOperator=NOP;
	if (m_operator_stack_top)
	{
		--m_operator_stack_top;
	}
	else
	{
		m_got_operators=false;
	}	
	
	//printf("m_operator_stack_top=%d\n",m_operator_stack_top);
}

void CExpressionEvaluator::add_new_operator(EScriptToken op)
{
	++m_operator_stack_top;
	if (m_operator_stack_top>=OPERATOR_STACK_SIZE)
	{
		set_error("Operator stack overflow");
		return;
	}
		
	mp_operator_stack[m_operator_stack_top].mOperator=op;	
	mp_operator_stack[m_operator_stack_top].mParenthesesCount=0;
	
	m_got_operators=true;
}

void CExpressionEvaluator::Input(EScriptToken op)
{
	while (m_got_operators)
	{
		if (mp_operator_stack[m_operator_stack_top].mParenthesesCount)
		{
			// The most recent operator is 'protected' by parentheses, 
			// so do not execute it.
			break;
		}	
	
		if (sSameOrLowerPrecedence(op,mp_operator_stack[m_operator_stack_top].mOperator))
		{
			// The new operator has the same or lower precedence than the last, so execute the
			// last operator.
			// Ie, in 2*3+5, we execute the * once we get the +
			execute_operation();
		}
		else
		{
			break;
		}	
	}	
				
	// Insert the new operator.
	add_new_operator(op);
}

void CExpressionEvaluator::Input(const CComponent *p_value)
{
	if (m_value_stack_top==0 && mp_value_stack[0].mType==ESYMBOLTYPE_NONE)
	{
	}
	else
	{
		++m_value_stack_top;
		if (m_value_stack_top>=VALUE_STACK_SIZE)
		{
			set_error("Value stack overflow");
			return;
		}
	}
		
	Dbg_MsgAssert(p_value,("NULL p_value"));
	CopyComponent(&mp_value_stack[m_value_stack_top],p_value);
}

void CExpressionEvaluator::OpenParenthesis()
{
	++mp_operator_stack[m_operator_stack_top].mParenthesesCount;
}

void CExpressionEvaluator::CloseParenthesis()
{
	while (true)
	{
		if (mp_operator_stack[m_operator_stack_top].mParenthesesCount)
		{
			--mp_operator_stack[m_operator_stack_top].mParenthesesCount;
			break;
		}
		else
		{
			execute_operation();
			if (mp_error_string)
			{
				break;
			}	
		}	
	}	
}

bool CExpressionEvaluator::GetResult(CComponent *p_result)
{
	Dbg_MsgAssert(p_result,("NULL p_result"));
	Dbg_MsgAssert(p_result->mType==ESYMBOLTYPE_NONE,("Non-empty component sent to GetResult, type = %s",GetTypeName(p_result->mType)));
	Dbg_MsgAssert(p_result->mUnion==0,("CComponent::mUnion not zero, type = %s",GetTypeName(p_result->mType)));
	
	while (true)
	{
		if (mp_operator_stack[m_operator_stack_top].mParenthesesCount)
		{
			set_error("Not enough close parentheses");
			break;
		}
			
		if (m_value_stack_top==0)
		{
			if (mp_operator_stack[0].mOperator!=0 || m_operator_stack_top)
			{
				set_error("Too many operators in expression");
				break;
			}	
			CopyComponent(p_result,&mp_value_stack[0]);
			CleanUpComponent(&mp_value_stack[0]);
			return true;
		}			
		
		execute_operation();
		if (mp_error_string)
		{
			break;
		}	
	}
	return false;
}

const char *CExpressionEvaluator::GetError()
{
	return mp_error_string;
}

} // namespace Script

