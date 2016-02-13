#ifndef __CORE_DEBUG_LOG_H
#define __CORE_DEBUG_LOG_H

namespace Log
{
void Init();
void AddEntry(char *p_fileName, int lineNumber, char *p_functionName, char *p_message=NULL);
}

#endif // #ifndef __CORE_DEBUG_LOG_H

