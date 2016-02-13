// Definition of the SkipToken function.
// This can be #included in the game code, and in PC utilities.
// It is in this file so that only this file needs to be updated when SkipToken
// needs to be modified.

// If included in PC code, then uint8 and Dbg_MsgAssert will need to be defined.

								   
// Returns a pointer to the next token after p_token.
// It won't necessarily skip over the complete format of the data that is expected
// to follow, it will just skip over enough that it returns a pointer to a token again.
// For example, ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE must be followed by a ESCRIPTTOKEN_PAIR,
// but SkipToken will not check for that and skip over the pair token too, it will just skip over the
// RANDOM_RANGE token and return a pointer to the ESCRIPTTOKEN_PAIR
// So if SkipToken is passed a pointer to a token, it is guaranteed to return a pointer to token, namely
// the nearest next one.
uint8 *SkipToken(uint8 *p_token)
{
    switch (*p_token)
    {
        case ESCRIPTTOKEN_ENDOFFILE:
            Dbg_MsgAssert(0,("Tried to skip past EndOfFile token"));
            break;
	    case ESCRIPTTOKEN_ENDOFLINE:
        case ESCRIPTTOKEN_EQUALS:
        case ESCRIPTTOKEN_DOT:
        case ESCRIPTTOKEN_COMMA:
        case ESCRIPTTOKEN_MINUS:
        case ESCRIPTTOKEN_ADD:
        case ESCRIPTTOKEN_DIVIDE:
        case ESCRIPTTOKEN_MULTIPLY:
        case ESCRIPTTOKEN_OPENPARENTH:
        case ESCRIPTTOKEN_CLOSEPARENTH:
        case ESCRIPTTOKEN_SAMEAS:
        case ESCRIPTTOKEN_LESSTHAN:
        case ESCRIPTTOKEN_LESSTHANEQUAL:
        case ESCRIPTTOKEN_GREATERTHAN:
        case ESCRIPTTOKEN_GREATERTHANEQUAL:
        case ESCRIPTTOKEN_STARTSTRUCT:
        case ESCRIPTTOKEN_STARTARRAY:
        case ESCRIPTTOKEN_ENDSTRUCT:
        case ESCRIPTTOKEN_ENDARRAY:
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
		case ESCRIPTTOKEN_KEYWORD_ALLARGS:
		case ESCRIPTTOKEN_ARG:
		case ESCRIPTTOKEN_OR:
		case ESCRIPTTOKEN_AND:
		case ESCRIPTTOKEN_XOR:
		case ESCRIPTTOKEN_SHIFT_LEFT:
		case ESCRIPTTOKEN_SHIFT_RIGHT:
		case ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE:
		case ESCRIPTTOKEN_KEYWORD_RANDOM_RANGE2:
		case ESCRIPTTOKEN_KEYWORD_NOT:
		case ESCRIPTTOKEN_KEYWORD_AND:
		case ESCRIPTTOKEN_KEYWORD_OR:
		case ESCRIPTTOKEN_KEYWORD_SWITCH:
		case ESCRIPTTOKEN_KEYWORD_ENDSWITCH:
		case ESCRIPTTOKEN_KEYWORD_CASE:
		case ESCRIPTTOKEN_KEYWORD_DEFAULT:
		case ESCRIPTTOKEN_COLON:
            ++p_token;
			break;
            
        case ESCRIPTTOKEN_NAME:
        case ESCRIPTTOKEN_INTEGER:
        case ESCRIPTTOKEN_HEXINTEGER:
        case ESCRIPTTOKEN_FLOAT:
	    case ESCRIPTTOKEN_ENDOFLINENUMBER:
		case ESCRIPTTOKEN_JUMP:
		case ESCRIPTTOKEN_RUNTIME_MEMBERFUNCTION:
		case ESCRIPTTOKEN_RUNTIME_CFUNCTION:
			p_token+=5;
            break;
        case ESCRIPTTOKEN_VECTOR:
            p_token+=13;
            break;
        case ESCRIPTTOKEN_PAIR:
            p_token+=9;
            break;
	    case ESCRIPTTOKEN_STRING:
        case ESCRIPTTOKEN_LOCALSTRING:
		{
            ++p_token;
			uint32 num_bytes=*p_token++;
			num_bytes+=(*p_token++)<<8;
			num_bytes+=(*p_token++)<<16;
			num_bytes+=(*p_token++)<<24;
            p_token+=num_bytes;
            break;
		}	
		case ESCRIPTTOKEN_CHECKSUM_NAME: 
			// Skip over the token and checksum.
			p_token+=5;
			// Skip over the string.
			while (*p_token)
			{
				++p_token;
			}
			// Skip over the terminator.
			++p_token;	
			break;			
		case ESCRIPTTOKEN_KEYWORD_RANDOM:
		case ESCRIPTTOKEN_KEYWORD_RANDOM2:
		case ESCRIPTTOKEN_KEYWORD_RANDOM_NO_REPEAT:
		case ESCRIPTTOKEN_KEYWORD_RANDOM_PERMUTE:
		{
            ++p_token;
			
			uint32 num_jumps=*p_token++;
			num_jumps+=(*p_token++)<<8;
			num_jumps+=(*p_token++)<<16;
			num_jumps+=(*p_token++)<<24;
			
			// Skip over all the weight & jump offsets.
			p_token+=2*num_jumps+4*num_jumps;
			break;
		}
			
        default:
            Dbg_MsgAssert(0,("Unrecognized script token sent to SkipToken()"));
            break;
    }
    return p_token;
}

