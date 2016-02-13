#ifndef	__SCRIPTING_EVAL_H
#define	__SCRIPTING_EVAL_H

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif

#ifndef	__SCRIPTING_COMPONENT_H
#include <gel/scripting/component.h>
#endif

#ifndef __SCRIPTING_TOKENS_H
#include <gel/scripting/tokens.h>
#endif

namespace Script
{

#define EVALUATOR_STRING_BUF_SIZE 200

#define NOP ((EScriptToken)0)

#define VALUE_STACK_SIZE 20
#define OPERATOR_STACK_SIZE VALUE_STACK_SIZE

struct SOperator
{
	EScriptToken mOperator;
	int mParenthesesCount;
};
	
class CExpressionEvaluator : public Spt::Class
{
	CComponent mp_value_stack[VALUE_STACK_SIZE];
	int m_value_stack_top;
	
	SOperator mp_operator_stack[OPERATOR_STACK_SIZE];
	int m_operator_stack_top;
	bool m_got_operators;

	// mp_token points to the position in the qb file being parsed.
	// The expression evaluator itself never does any parsing of the qb data,
	// it only stores this so that it can print the line number if an error occurs.
	uint8 *mp_token;	
	const char *mp_error_string;
	
	// Sometimes, if an expression contains an error such as a missing parameter value, we
	// do not want it to assert or print a warning message, but just return a void value instead.
	// For example, when scanning through all the scripts looking for calls to EndGap, the
	// function AddComponentsUntilEndOfLine is used, but often parameters are missing then
	// because the script is not being run, just scanned through. Don't wan't it to stop with
	// an error in that case.
	bool m_errors_enabled;
	
	void set_error(const char *p_error);
	void execute_operation();
	void add_new_operator(EScriptToken op);
									 
public:	
	CExpressionEvaluator();
	~CExpressionEvaluator();
	
	void EnableErrorChecking() {m_errors_enabled=true;}
	void DisableErrorChecking() {m_errors_enabled=false;}
	bool ErrorCheckingEnabled() {return m_errors_enabled;}
							 
	// These are not defined, just declared so that the code won't link if they
	// are attempted to be used.
	CExpressionEvaluator( const CExpressionEvaluator& rhs );
	CExpressionEvaluator& operator=( const CExpressionEvaluator& rhs );

	void Clear();	
	void ClearIfNeeded();
	void Input(EScriptToken op);
	void Input(const CComponent *p_value);
	void OpenParenthesis();
	void CloseParenthesis();
	bool GetResult(CComponent *p_result);
	const char *GetError();
	void SetTokenPointer(uint8 *p_token);
};
	
} // namespace Script

#endif // #ifndef	__SCRIPTING_EVAL_H

