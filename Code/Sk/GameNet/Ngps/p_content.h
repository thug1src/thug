/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 2002 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate4													**
**																			**
**	Module:			GameNet													**
**																			**
**	File name:		p_content.h												**
**																			**
**	Created:		05/28/02	- spg										**
**																			**
*****************************************************************************/

#ifndef	__P_CONTENT_H
#define	__P_CONTENT_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>

namespace GameNet
{

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

enum
{
	vCONTENT_STATE_IDLE,
	vCONTENT_STATE_GETTING_DIR_LIST,
	vCONTENT_STATE_GETTING_SUB_DIR_LIST,
	vCONTENT_STATE_DOWNLOADING,
	vCONTENT_STATE_UPLOADING,
	vCONTENT_STATE_GETTING_UPLOADED_LEVELS,
	vCONTENT_STATE_DOWNLOADING_FACE
};

enum
{
	vCONTENT_RESULT_UNDETERMINED,
	vCONTENT_RESULT_SUCCESS,
	vCONTENT_RESULT_FAILURE
};

/*****************************************************************************
**							     Class Definitions							**
*****************************************************************************/

class Content : public Lst::Node< Content >
{
public:
	enum
	{
		vMAX_FILE_NAME_LEN = 23,
		vMAX_PATH_NAME_LEN = 127,
		vMAX_CREATOR_NAME_LEN = 15,
		vMAX_DIMENSION_LEN = 15,
		vMAX_CHECKSUM_LEN = 32,
		vMAX_DESC_LEN = 255,
		vMAX_NUM_LEN = 15,
		vMAX_THEME_NAME_LEN = 23,
		vMAX_SCRIPT_NAME_LEN = 23,
		vMAX_SEX_NAME_LEN = 15,
	};

	Content( void );

	char	m_Path[vMAX_PATH_NAME_LEN + 1];
	char	m_Name[vMAX_FILE_NAME_LEN + 1];
	char	m_Creator[vMAX_CREATOR_NAME_LEN + 1];
	char	m_Size[vMAX_DIMENSION_LEN + 1];
	char	m_Checksum[vMAX_CHECKSUM_LEN + 1];
	char	m_Description[vMAX_DESC_LEN + 1];
	int		m_Score;
	int		m_Width;
	int		m_Height;
	int		m_NumPieces;
	int		m_NumGaps;
	int		m_NumGoals;
	int		m_Theme;
	uint32	m_TODScript;
	int		m_IsMale;
};

class Directory : public Lst::Node< Directory >
{
public:
	enum
	{
		vMAX_DIR_NAME_LEN = 23
	};

	Directory( void );
	~Directory( void );

	bool	m_Uploads;
	char	m_Name[vMAX_DIR_NAME_LEN + 1];
	Lst::Head< Content > 	m_content_list;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class DownloadContext
{
public:
	int						m_max_size;
	char					m_path[256];
	ghttpCompletedCallback	m_dl_complete;
	uint32					m_transfer_failed_script;
	uint32					m_transfer_complete_script;
	uint32					m_update_progress_script;
	uint32					m_file_not_found_script;

};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class ContentMan
{
public:
					ContentMan( void );
					~ContentMan( void );

	void			DownloadDirectoryListing( void );
	void			FillDirectoryListing( char* dir_text );
	void			FreeDirectoryListing( void );
	void			GetDirListing( char* path );
	void			GetSubDirListing( Directory* dir );
	void			DownloadFace( char* filename );
	void			DownloadFile( DownloadContext* context );

	void			SetPercentComplete( int percent_complete );
	void			SetResult( int result );
	void			SetError( int error );

	static	bool	ScriptDownloadFace(Script::CScriptStructure *pParams, Script::CScript *pScript);

    static	bool	ScriptDownloadDirectoryList(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool	ScriptFreeDirectoryListing(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool	ScriptDownloadFile(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool	ScriptUploadFile(Script::CScriptStructure *pParams, Script::CScript *pScript);

	static	bool	ScriptFillVaultMenu(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool	ScriptNextVaultCategory(Script::CScriptStructure *pParams, Script::CScript *pScript);
	static	bool	ScriptPrevVaultCategory(Script::CScriptStructure *pParams, Script::CScript *pScript);

private:

	Tsk::Task< ContentMan >*				m_http_transfer_task;
	static	Tsk::Task< ContentMan >::Code   s_http_transfer_code;

	static	void	s_threaded_upload_file( ContentMan* content_man );
	static	void	s_threaded_download_file( const char* path );
	static	void	s_threaded_download_face( const char* path );
	static	void	s_threaded_download_directory_list( ContentMan* content_man );

	static	void 	s_progress_callback(	GHTTPRequest request,       // The request.
											GHTTPState state,           // The current state of the request.
											const char* buffer,        	// The file's bytes so far, NULL if state<GHTTPReceivingFile.
											int buffer_len,             // The number of bytes in the buffer, 0 if state<GHTTPReceivingFile.
											int bytes_received,         // The total number of bytes receivied, 0 if state<GHTTPReceivingFile.
											int total_size,             // The total size of the file, -1 if unknown.
											void * param                // User-data.
										);
	static	GHTTPBool s_ul_completed_callback(	GHTTPRequest request,       // The request.
											GHTTPResult result,         // The result (success or an error).
											char* buffer,              // The file's bytes (only valid if ghttpGetFile[Ex] was used).
											int buffer_len,              // The file's length.
											void * param                // User-data.
											);
	static	GHTTPBool s_dl_completed_callback(	GHTTPRequest request,       // The request.
											GHTTPResult result,         // The result (success or an error).
											char* buffer,              // The file's bytes (only valid if ghttpGetFile[Ex] was used).
											int buffer_len,              // The file's length.
											void * param                // User-data.
											);
	static	GHTTPBool s_face_dl_completed_callback(	GHTTPRequest request,       // The request.
											GHTTPResult result,         // The result (success or an error).
											char* buffer,              // The file's bytes (only valid if ghttpGetFile[Ex] was used).
											int buffer_len,              // The file's length.
											void * param                // User-data.
											);
	int			m_state;
	int			m_percent_complete;
	int			m_result;
	int			m_error;
	int			m_buffer_size;
	char* 		m_buffer;
	Tmr::Time	m_last_comm_time;
	char		m_download_path[128];
	char		m_upload_filename[128];
	char		m_upload_description[512];
	int			m_thread_semaphore_id;
	GHTTPRequest m_file_request;
	Script::CStruct* m_upload_struct;
	
	int			m_Score;
	int			m_Width;
	int			m_Height;
	int			m_NumPieces;
	int			m_NumGaps;
	int			m_NumGoals;
	int			m_Theme;
	uint32		m_TODScript;
	int			m_IsMale;

	Lst::Head< Directory > m_directories;
	Lst::Search< Directory > m_next_dir_sh;
	Directory	*m_cur_dir;
	uint32		m_vault_type;
	char		m_vault_root[64];

	uint32		m_transfer_failed_script;
	uint32		m_transfer_complete_script;
	uint32		m_update_progress_script;
	uint32		m_file_not_found_script;
	ghttpCompletedCallback	m_dl_complete;
};

/*****************************************************************************
**							 Private Declarations							**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							  Public Declarations							**
*****************************************************************************/


/*****************************************************************************
**							   Public Prototypes							**
*****************************************************************************/


/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

}	// namespace GameNet

#endif	// __P_CONTENT_H