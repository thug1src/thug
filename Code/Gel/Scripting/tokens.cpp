///////////////////////////////////////////////////////////////////////////////////////
//
// tokens.cpp		KSH 22 Oct 2001
//
// Just a function for getting a token name.
//
///////////////////////////////////////////////////////////////////////////////////////

#include <gel/scripting/tokens.h>
#ifndef __CORE_DEFINES_H
#include <core/defines.h> // For __NOPT_ASSERT__
#endif

namespace Script
{

#ifdef __NOPT_ASSERT__
const char *GetTokenName(EScriptToken token)
{
	switch (token)
	{
	case ESCRIPTTOKEN_ENDOFFILE:
		return "ENDOFFILE";
		break;
	case ESCRIPTTOKEN_ENDOFLINE:
		return "ENDOFLINE";
		break;
	case ESCRIPTTOKEN_ENDOFLINENUMBER:
		return "ENDOFLINENUMBER";
		break;
	case ESCRIPTTOKEN_STARTSTRUCT:
		return "{";
		break;
	case ESCRIPTTOKEN_ENDSTRUCT:
		return "}";
		break;
	case ESCRIPTTOKEN_STARTARRAY:
		return "[";
		break;
	case ESCRIPTTOKEN_ENDARRAY:
		return "]";
		break;
	case ESCRIPTTOKEN_EQUALS:
		return "=";
		break;
	case ESCRIPTTOKEN_DOT:
		return ".";
		break;
	case ESCRIPTTOKEN_COMMA:
		return ",";
		break;
	case ESCRIPTTOKEN_MINUS:
		return "-";
		break;
	case ESCRIPTTOKEN_ADD:
		return "+";
		break;
	case ESCRIPTTOKEN_DIVIDE:
		return "/";
		break;
	case ESCRIPTTOKEN_MULTIPLY:
		return "*";
		break;
	case ESCRIPTTOKEN_OPENPARENTH:
		return "(";
		break;
	case ESCRIPTTOKEN_CLOSEPARENTH:
		return ")";
		break;
	case ESCRIPTTOKEN_DEBUGINFO:
		return "DEBUGINFO";
		break;
	case ESCRIPTTOKEN_SAMEAS:
		return "==";
		break;
	case ESCRIPTTOKEN_LESSTHAN:
		return "<";
		break;
	case ESCRIPTTOKEN_LESSTHANEQUAL:
		return "<=";
		break;
	case ESCRIPTTOKEN_GREATERTHAN:
		return ">";
		break;
	case ESCRIPTTOKEN_GREATERTHANEQUAL:
		return ">=";
		break;
	case ESCRIPTTOKEN_NAME:
		return "NAME";
		break;
	case ESCRIPTTOKEN_INTEGER:
		return "INTEGER";
		break;
	case ESCRIPTTOKEN_HEXINTEGER:
		return "HEXINTEGER";
		break;
    case ESCRIPTTOKEN_ENUM:
		return "ENUM";
		break;
	case ESCRIPTTOKEN_FLOAT:
		return "FLOAT";
		break;
	case ESCRIPTTOKEN_STRING:
		return "STRING";
		break;
	case ESCRIPTTOKEN_LOCALSTRING:
		return "LOCALSTRING";
		break;
	case ESCRIPTTOKEN_ARRAY:
		return "ARRAY";
		break;
	case ESCRIPTTOKEN_VECTOR:
		return "VECTOR";
		break;
	case ESCRIPTTOKEN_PAIR:
		return "PAIR";
		break;
	case ESCRIPTTOKEN_KEYWORD_BEGIN:
		return "BEGIN";
		break;
	case ESCRIPTTOKEN_KEYWORD_REPEAT:
		return "REPEAT";
		break;
	case ESCRIPTTOKEN_KEYWORD_BREAK:
		return "BREAK";
		break;
	case ESCRIPTTOKEN_KEYWORD_SCRIPT:
		return "SCRIPT";
		break;
	case ESCRIPTTOKEN_KEYWORD_ENDSCRIPT:
		return "ENDSCRIPT";
		break;
	case ESCRIPTTOKEN_KEYWORD_IF:
		return "IF";
		break;
	case ESCRIPTTOKEN_KEYWORD_ELSE:
		return "ELSE";
		break;
	case ESCRIPTTOKEN_KEYWORD_ELSEIF:
		return "ELSEIF";
		break;
	case ESCRIPTTOKEN_KEYWORD_ENDIF:
		return "ENDIF";
		break;
	case ESCRIPTTOKEN_KEYWORD_RETURN:
		return "RETURN";
		break;
	case ESCRIPTTOKEN_KEYWORD_NOT:
		return "NOT";
		break;
	case ESCRIPTTOKEN_KEYWORD_SWITCH:
		return "SWITCH";
		break;
	case ESCRIPTTOKEN_KEYWORD_ENDSWITCH:
		return "ENDSWITCH";
		break;
	case ESCRIPTTOKEN_KEYWORD_CASE:
		return "CASE";
		break;
	case ESCRIPTTOKEN_KEYWORD_DEFAULT:
		return "DEFAULT";
		break;
    case ESCRIPTTOKEN_UNDEFINED:
		return "UNDEFINED";
		break;
	case ESCRIPTTOKEN_CHECKSUM_NAME:
		return "CHECKSUM_NAME";
		break;
	case ESCRIPTTOKEN_KEYWORD_ALLARGS:
		return "<...>";
		break;
	case ESCRIPTTOKEN_ARG:	
		return "ARG";
		break;
		
	case ESCRIPTTOKEN_KEYWORD_RANDOM:
		return "RANDOM";
		break;

	case ESCRIPTTOKEN_KEYWORD_RANDOM2:
		return "RANDOM2";
		break;
		
	case ESCRIPTTOKEN_KEYWORD_RANDOM_NO_REPEAT:
		return "RANDOM_NO_REPEAT";
		break;

	case ESCRIPTTOKEN_KEYWORD_RANDOM_PERMUTE:
		return "RANDOM_PERMUTE";
		break;
		
	case ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE:
		return "RANDOM_RANGE";
		break;

	case ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE2:
		return "RANDOM_RANGE2";
		break;
			
	case ESCRIPTTOKEN_OR:
		return "OR";
		break;
	case ESCRIPTTOKEN_AND:
		return "AND";
		break;
	case ESCRIPTTOKEN_XOR:
		return "XOR";
		break;
	
	case ESCRIPTTOKEN_SHIFT_LEFT:
		return "SHIFT_LEFT";
		break;
	case ESCRIPTTOKEN_SHIFT_RIGHT:
		return "SHIFT_RIGHT";
		break;
		
	case ESCRIPTTOKEN_COLON:
		return "COLON";
		break;
		
	case ESCRIPTTOKEN_RUNTIME_CFUNCTION:
		return "RUNTIME-CFUNCTION";
		break;
		
	case ESCRIPTTOKEN_RUNTIME_MEMBERFUNCTION:
		return "RUNTIME-MEMBERFUNCTION";
		break;
			
	default:
		return "Unknown";
		break;
	}		
}
#endif // #ifdef __NOPT_ASSERT__

} // namespace Script

