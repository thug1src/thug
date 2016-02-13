/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2001 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:																**
**																			**
**	Module:									 								**
**																			**
**	File name:																**
**																			**
**	Created by:		rjm, 1/23/2001											**
**																			**
**	Description:						 									**
**																			**
*****************************************************************************/

#ifndef __CORE_FILE_PRE_H
#define __CORE_FILE_PRE_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sys/file/AsyncTypes.h>

namespace Lst
{
	template<class _V> class StringHashTable;
}

namespace Script
{
	class CStruct;
	class CScript;
}	

/*****************************************************************************
**								   Defines									**
*****************************************************************************/


namespace File
{

						


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/

bool ScriptInPreFile(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptLoadPreFile(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptUnloadPreFile(Script::CStruct *pParams, Script::CScript *pScript);

bool ScriptIsLoadPreFinished(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptAllLoadPreFinished(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptWaitLoadPre(Script::CStruct *pParams, Script::CScript *pScript);
bool ScriptWaitAllLoadPre(Script::CStruct *pParams, Script::CScript *pScript);

/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/


/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class CAsyncFileHandle;
class PreMgr;
class  PreFile  : public Spt::Class
{
	

public:

	// The actual PRE file (it is public so that FileHandle can be made public).
	struct _File
	{
		int				compressedDataSize;
		uint8 *			pCompressedData;
		uint8 *			pData;
		int				m_position;
		int				m_filesize;
	};

	// Just typedef a file handle type since the file itself contains all the info
	typedef _File FileHandle;

	PreFile(uint8 *p_file_buffer, bool useBottomUpHeap=false);
	~PreFile();
	
	
	
	bool 		FileExists(const char *pName);
	void*		LoadContainedFile(const char *pName,int *p_size, void *p_dest = NULL);
	FileHandle*	GetContainedFile(const char *pName);
	uint8*		GetContainedFileByHandle(FileHandle *pHandle);

	void Reset();
	uint32 Read(void *addr, uint32 count);
	int Eof();
	void Open(bool async);
	void Close(bool async);
	int Seek(long offset, int origin);
	int TellActive( void )				{ if( mp_activeFile ){ return mp_activeFile->m_position; }else{ return 0; }}
	int GetFileSize( void )				{ if( mp_activeFile ){ return mp_activeFile->m_filesize; }else{ return 0; }}
	int GetFilePosition( void )			{ if( mp_activeFile ){ return mp_activeFile->m_position; }else{ return 0; }}

private:

	static void						s_delete_file(_File *pFile, void *pData);
	
	uint8 *							mp_buffer;
	int								m_numEntries;
	
	// maps filenames to pointers
	Lst::StringHashTable<_File> *	mp_table;

	_File *							mp_activeFile;

	int								m_numOpenAsyncFiles;
	
	bool							m_use_bottom_up_heap;
};




class  PreMgr  : public Spt::Class
{
	
	DeclareSingletonClass(PreMgr);

public:
	bool InPre(const char *pFilename);
	void LoadPre(const char *pFilename, bool async, bool dont_assert = false, bool useBottomUpHeap=false);
	void LoadPrePermanently(const char *pFilename, bool async, bool dont_assert = false);
	void UnloadPre(const char *pFilename, bool dont_assert = false);
	void * LoadFile(const char *pName, int *p_size, void *p_dest = NULL);

	// Async check functions
	bool IsLoadPreFinished(const char *pFilename);
	bool AllLoadPreFinished();
	void WaitLoadPre(const char *pFilename);
	void WaitAllLoadPre();

	static bool sPreEnabled();
	static bool sPreExecuteSuccess();
	
	static bool					pre_fexist(const char *name);
	static PreFile::FileHandle *pre_fopen(const char *name, const char *access, bool async = false);
	static int					pre_fclose(PreFile::FileHandle *fptr, bool async = false);
	static size_t				pre_fread(void *addr, size_t size, size_t count, PreFile::FileHandle *fptr);
	static size_t				pre_fwrite(const void *addr, size_t size, size_t count, PreFile::FileHandle *fptr);
	static char *				pre_fgets(char *buffer, int maxLen, PreFile::FileHandle *fptr);
	static int					pre_fputs(const char *buffer, PreFile::FileHandle *fptr);
	static int					pre_feof(PreFile::FileHandle *fptr);
	static int					pre_fseek(PreFile::FileHandle *fptr, long offset, int origin);
	static int					pre_fflush(PreFile::FileHandle *fptr);
	static int					pre_ftell(PreFile::FileHandle *fptr);
	static int					pre_get_file_size(PreFile::FileHandle *fptr);
	static int					pre_get_file_position(PreFile::FileHandle *fptr);

private:
	// Constants
	enum
	{
		MAX_COMPACT_FILE_NAME = 64,				// Only store the right-most characters of the filename to save space
		MAX_NUM_ASYNC_LOADS = 8,
	};

	// Holds data for pending async loads
	struct SPendingAsync
	{
		CAsyncFileHandle *			mp_file_handle;
		char						m_file_name[MAX_COMPACT_FILE_NAME];
	};

	PreMgr();
	~PreMgr();

	void				 loadPre(const char *pFilename, bool async, bool dont_assert = false, bool useBottomUpHeap=false);
    void    			 postLoadPre(CAsyncFileHandle *p_file_handle, uint8 *pData, int size);
	bool				 fileExists(const char *pName);
	PreFile::FileHandle *getContainedFile(const char *pName);
	uint8 *				 getContainedFileByHandle(PreFile::FileHandle *pHandle);

	static char *		 getCompactFileName(char *pName);		// Returns point in string where it will fit in compact space

	static void			 async_callback(CAsyncFileHandle *p_file_handle, EAsyncFunctionType function,
										int result, unsigned int arg0, unsigned int arg1);

	PreFile							*mp_activePre;
	PreFile::FileHandle *	 		mp_activeHandle;
	uint8 *							mp_activeData;
	// handle of current file being accessed from regular file system, for quick check
	void *							mp_activeNonPreHandle;

	static bool						s_lastExecuteSuccess;
	static PreMgr *					sp_mgr;
	
	Lst::StringHashTable<PreFile> *	mp_table;

	bool							m_blockPreLoading;

	// Async status
	SPendingAsync					m_pending_pre_files[MAX_NUM_ASYNC_LOADS];
	int								m_num_pending_pre_files;
};


} // namespace File

#endif	// __CORE_FILE_PRE_H


