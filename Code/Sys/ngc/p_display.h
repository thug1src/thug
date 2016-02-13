#ifndef _DISPLAY_H_
#define _DISPLAY_H_

typedef void (*NsDisplay_StartRenderingCallback)(void);
typedef void (*NsDisplay_EndRenderingCallback)(void);

namespace NsDisplay
{
	void			init					( void );

	void			begin					( void );
	void			end						( bool clear = true );

	void			setBackgroundColor		( GXColor color );

	void			setRenderStartCallback	( NsDisplay_StartRenderingCallback pCB );
	void			setRenderEndCallback	( NsDisplay_EndRenderingCallback pCB );

	void			flush					( void );

	int				getCurrentBufferIndex	( void );

	bool			shouldReset				( void );
	void			doReset					( bool hard_reset = false, bool forceMenu = false );

	void			Check480P				( void );
	void			Check60Hz				( void );
};

void display_legal( void );

namespace Script
{
	class CScriptStructure;
	class CScript;
}

namespace Nx
{

bool ScriptNgc_BGColor(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptNgc_Message(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptNgc_Menu(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptNgc_Set480P(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptNgc_Set480I(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptNgc_Set60Hz(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptNgc_Set50Hz(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptNgc_SetWide(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptNgc_SetStandard(Script::CScriptStructure *pParams, Script::CScript *pScript);
bool ScriptNgc_ReduceColors(Script::CScriptStructure *pParams, Script::CScript *pScript);

}

#endif		// _DISPLAY_H_

