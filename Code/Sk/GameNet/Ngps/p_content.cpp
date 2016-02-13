/*****************************************************************************
**																			**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 2000 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		Skate4													**
**																			**
**	Module:			GameNet					 								**
**																			**
**	File name:		Lobby.cpp												**
**																			**
**	Created by:		04/4/02	-	spg											**
**																			**
**	Description:	Gamespy peer lobby implementation						**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/defines.h>
#include <core/thread.h>
#include <core/singleton.h>

#include <gel/mainloop.h>

#include <gel/scripting/script.h>
#include <gel/scripting/utils.h>

#include <sk/gamenet/gamenet.h>
#include <sk/gamenet/ngps/p_content.h>
#include <sk/gamenet/ngps/p_buddy.h>
#include <sk/scripting/cfuncs.h>
#include <sk/scripting/mcfuncs.h>
#include <sk/ParkEditor2/EdMap.h>

// for downloading faces
#include <sk/modules/skate/skate.h>
#include <sk/objects/skaterprofile.h>
#include <gfx/facetexture.h>
#include <gfx/modelappearance.h>

#include <ghttp/ghttp.h>
#include <ghttp/ghttppost.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/
namespace GameNet
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#define vHTTP_COMM_TIMEOUT	Tmr::Seconds( 25 )
#define vMAX_FACE_SIZE		( 32 * 1024 )
#define vMAX_FILE_SIZE	( 80 * 1024 )

/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/

/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/

/*****************************************************************************
**							  Private Functions								**
*****************************************************************************/


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void 	ContentMan::s_progress_callback(	GHTTPRequest request,       // The request.
											GHTTPState state,           // The current state of the request.
											const char* buffer,        	// The file's bytes so far, NULL if state<GHTTPReceivingFile.
											int buffer_len,             // The number of bytes in the buffer, 0 if state<GHTTPReceivingFile.
											int bytes_received,         // The total number of bytes receivied, 0 if state<GHTTPReceivingFile.
											int total_size,             // The total size of the file, -1 if unknown.
											void * param                // User-data.
										)
{
	ContentMan* content_man;

	content_man = (ContentMan*) param;

	Dbg_Printf( "state: %d buffer len: %d, bytes received %d, total_size %d\n", state, buffer_len, bytes_received, total_size );
	if(	( state == GHTTPReceivingFile ) ||
		( state == GHTTPPosting ))
	{
		if( total_size > 0 )
		{
			content_man->m_percent_complete = (int) (((float) bytes_received / (float) total_size ) * 100.0f );
		}
	}

	content_man->m_last_comm_time = Tmr::GetTime();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

GHTTPBool ContentMan::s_dl_completed_callback(	GHTTPRequest request,       // The request.
										GHTTPResult result,         // The result (success or an error).
										char* buffer,              // The file's bytes (only valid if ghttpGetFile[Ex] was used).
										int buffer_len,              // The file's length.
										void * param                // User-data.
										)
{
	ContentMan* content_man;

	content_man = (ContentMan*) param;

	Dbg_Printf( "Got result %d\n", result );

	content_man->m_percent_complete = 100;
	if( result == GHTTPSuccess )
	{
		uint32 type;

		content_man->m_result = vCONTENT_RESULT_SUCCESS;

		type = 0;
		switch( content_man->m_vault_type ) 
		{
			case 0x5be5569f:	// parks
				type = CRCD(0x3bf882cc,"park");
				break;
			case 0x7364ea1:		// skaters
				type = CRCD(0xb010f357,"optionsandpros");
				break;
			case 0x38dbe1d0:	// goals
				type = CRCD(0x62896edf,"createdgoals");
				break;
			case 0x1e26fd3e:	// tricks
				type = CRCD(0x61a1bc57,"cat");
				break;
			default:
				Dbg_Assert( 0 );
				break;
		}
		CFuncs::SetVaultData((uint8*) buffer, type );
		
		content_man->m_error = 0;
	}
	else
	{
		content_man->m_result = vCONTENT_RESULT_FAILURE;
		content_man->m_error = result;
	}
	
	return GHTTPTrue;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

GHTTPBool ContentMan::s_face_dl_completed_callback(	GHTTPRequest request,       // The request.
													GHTTPResult result,         // The result (success or an error).
													char* buffer,              // The file's bytes (only valid if ghttpGetFile[Ex] was used).
													int buffer_len,              // The file's length.
													void * param                // User-data.
													)
{
	ContentMan* content_man;

	content_man = (ContentMan*) param;

	Dbg_Printf( "Got result %d\n", result );

	content_man->m_percent_complete = 100;
	if( result == GHTTPSuccess )
	{
		content_man->m_result = vCONTENT_RESULT_SUCCESS;
		content_man->m_error = 0;
		
		// Copy the downloaded data to the skater profile
		Mdl::Skate* skate_mod = Mdl::Skate::Instance();
		Obj::CSkaterProfile* pSkaterProfile = skate_mod->GetCurrentProfile();
		Dbg_Assert( pSkaterProfile );
		Dbg_MsgAssert( !pSkaterProfile->IsPro(), ( "Can only download face onto a custom skater.  UI must make the custom skater active before this point" ) );
		Gfx::CModelAppearance* pAppearance = pSkaterProfile->GetAppearance();
		Dbg_Assert( pAppearance );
		Gfx::CFaceTexture* pFaceTexture = pAppearance->GetFaceTexture();
		Dbg_MsgAssert( pFaceTexture, ( "Appearance has no face texture" ) );
		
		// GJ:  The CFaceTexture is permanently allocated in the
		// custom skater, so there should be no need to push 
		// any heap contexts here...  if this assumption changes, 
		// then we should probably push the skt_info heap
		pFaceTexture->ReadTextureDataFromBuffer( (uint8*)buffer, 0 );
		pFaceTexture->SetValid( true );

		// rebuild the skater to display the new face
		Script::RunScript( CRCD(0x808008ea,"refresh_skater_model") );
	}
	else
	{
		content_man->m_result = vCONTENT_RESULT_FAILURE;
		content_man->m_error = result;
	}
    
	return GHTTPTrue;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

GHTTPBool ContentMan::s_ul_completed_callback(	GHTTPRequest request,       // The request.
										GHTTPResult result,         // The result (success or an error).
										char* buffer,              // The file's bytes (only valid if ghttpGetFile[Ex] was used).
										int buffer_len,              // The file's length.
										void * param                // User-data.
										)
{
	ContentMan* content_man;

	content_man = (ContentMan*) param;

	Dbg_Printf( "Got result %d\n", result );

	content_man->m_percent_complete = 100;
	if( result == GHTTPSuccess )
	{
		content_man->m_result = vCONTENT_RESULT_SUCCESS;
	}
	else
	{
		content_man->m_result = vCONTENT_RESULT_FAILURE;
	}
	
	return GHTTPTrue;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	ContentMan::s_http_transfer_code( const Tsk::Task< ContentMan > &task )
{
	ContentMan& content_man = task.GetData();

	if(( Tmr::GetTime() - content_man.m_last_comm_time ) > vHTTP_COMM_TIMEOUT )
	{
		if(	content_man.m_state != vCONTENT_STATE_IDLE )
		{
			delete [] content_man.m_buffer;
		}

		Dbg_Printf( "********* TIMING OUT!!!!\n" );
		Dbg_Assert( content_man.m_transfer_failed_script );

		ghttpCancelRequest( content_man.m_file_request );

		Script::RunScript( content_man.m_transfer_failed_script );
		content_man.m_state = vCONTENT_STATE_IDLE;
		content_man.m_http_transfer_task->Remove();

        //ghttpCleanup();
		return;
	}

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());
	ghttpThink();
	Mem::Manager::sHandle().PopContext();

	if( content_man.m_percent_complete < 100 )
	{
		if(( content_man.m_state == vCONTENT_STATE_DOWNLOADING ) ||
		   ( content_man.m_state == vCONTENT_STATE_DOWNLOADING_FACE ))
		{
			Script::CStruct* p_item_params;
			char pct_string[8];
		
			sprintf( pct_string, "%d%%", content_man.m_percent_complete );
					
			p_item_params = new Script::CStruct;
			p_item_params->AddComponent( Script::GenerateCRC( "text" ), ESYMBOLTYPE_STRING, pct_string );
			Dbg_Assert( content_man.m_update_progress_script );
			Script::RunScript( content_man.m_update_progress_script, p_item_params );
			delete p_item_params;
		}
	}
	else
	{
		if( ( content_man.m_state == vCONTENT_STATE_DOWNLOADING ) ||
			( content_man.m_state == vCONTENT_STATE_DOWNLOADING_FACE ))
		{
			delete [] content_man.m_buffer;
		}

		//WaitSema( content_man.m_thread_semaphore_id );
		if(( content_man.m_state == vCONTENT_STATE_DOWNLOADING ) || 
		   ( content_man.m_state == vCONTENT_STATE_DOWNLOADING_FACE ) ||
		   ( content_man.m_state == vCONTENT_STATE_UPLOADING ))
		{
			int prev_state;
			 
			prev_state = content_man.m_state;
			content_man.m_http_transfer_task->Remove();
			content_man.m_state = vCONTENT_STATE_IDLE;

			if( content_man.m_result == vCONTENT_RESULT_SUCCESS )
			{
				Script::CStruct* params;

				params = new Script::CStruct;
				params->AddChecksum( "type", content_man.m_vault_type );
				if( prev_state == vCONTENT_STATE_UPLOADING )
				{
					Script::RunScript( "LaunchUploadCompleteDialog", params );
				}
				else
				{
					Dbg_Assert( content_man.m_transfer_complete_script );
					Script::RunScript( content_man.m_transfer_complete_script, params );
				}

				delete params;
			}
			else
			{
				if( content_man.m_error == GHTTPFileNotFound )
				{
					Dbg_Assert( content_man.m_file_not_found_script );
					Script::RunScript( content_man.m_file_not_found_script );
				}
				else
				{
					Dbg_Assert( content_man.m_transfer_failed_script );
					Script::RunScript( content_man.m_transfer_failed_script );
				}
			}
			if( ( prev_state == vCONTENT_STATE_DOWNLOADING ) ||
				( prev_state == vCONTENT_STATE_DOWNLOADING_FACE ))
			{
				//delete [] content_man.m_buffer;
			}
			if( prev_state == vCONTENT_STATE_UPLOADING )
			{
				delete [] content_man.m_buffer;
			}
			
			//content_man.m_state = vCONTENT_STATE_IDLE;
			//content_man.m_http_transfer_task->Remove();
			//ghttpCleanup();
		}
		else if( content_man.m_state == vCONTENT_STATE_GETTING_DIR_LIST )
		{
			if( content_man.m_result == vCONTENT_RESULT_SUCCESS )
			{
				char* token;
				Directory* dir;

				Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

				Dbg_Printf( "**** Parsing directory listing\n" );
				token = strtok( content_man.m_buffer, "\r\n" );
				if( token )
				{
					do
					{   
						Dbg_Printf( "**** Got dir %s\n", token );
						dir = new Directory;
						content_man.m_directories.AddToHead( dir );
						strcpy( dir->m_Name, token );
					} while(( token = strtok( NULL, "\r\n" )));

					content_man.m_cur_dir = content_man.m_next_dir_sh.FirstItem( content_man.m_directories );
					if( content_man.m_cur_dir->m_Uploads )
					{
						content_man.m_cur_dir = content_man.m_next_dir_sh.NextItem();
					}

					Dbg_Printf( "**** Getting subdir listing for %s\n", content_man.m_cur_dir->m_Name );

					content_man.m_state = vCONTENT_STATE_IDLE;
					//ghttpCleanup();
					delete [] content_man.m_buffer;

					//SignalSema( content_man.m_thread_semaphore_id );
					content_man.GetSubDirListing( content_man.m_cur_dir );
					Mem::Manager::sHandle().PopContext();
					return;
				}

				Mem::Manager::sHandle().PopContext();
			}
			else
			{
				content_man.m_state = vCONTENT_STATE_IDLE;
				content_man.m_http_transfer_task->Remove();
				//ghttpCleanup();
				delete [] content_man.m_buffer;

				Dbg_Assert( content_man.m_transfer_failed_script );
				Script::RunScript( content_man.m_transfer_failed_script );
			}
		}
		else if( content_man.m_state == vCONTENT_STATE_GETTING_SUB_DIR_LIST )
		{
			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

			Dbg_Printf( "Got sub_dir list\n" );

			if( content_man.m_result == vCONTENT_RESULT_SUCCESS )
			{
				Content* content;
				char* line, *colon, *token;
				char fname[Content::vMAX_FILE_NAME_LEN + 1];
				char buff[128];

				line = strtok( content_man.m_buffer, "\r\n" );
				if( line )
				{
					do
					{
						content = new Content;
						content_man.m_cur_dir->m_content_list.AddToHead( content );

						token = line;
						colon = strstr( line, ":" );
						Dbg_Assert( colon );
						*colon = '\0';
						strcpy( fname, token );

						sprintf( content->m_Path, "http://www.thugonline.com/%s/%s/%s", content_man.m_vault_root, content_man.m_cur_dir->m_Name, fname );
						Dbg_Printf( "Filename: %s\n", fname );

						token = colon + 1;
						colon = strstr( token, ":" );
						Dbg_Assert( colon );
						*colon = '\0';
						strcpy( content->m_Name, token );
						Dbg_Printf( "Display name: %s\n", content->m_Name );

						token = colon + 1;
						colon = strstr( token, ":" );
						Dbg_Assert( colon );
						*colon = '\0';
						strcpy( content->m_Creator, token );
						Dbg_Printf( "Creator: %s\n", content->m_Creator );

						// Read the content size, if we're looking at parks
						if( content_man.m_vault_type == CRCD(0x5be5569f,"parks") )
						{
							token = colon + 1;
							colon = strstr( token, ":" );
							Dbg_Assert( colon );
							*colon = '\0';
							strcpy( content->m_Size, token );
							Dbg_Printf( "Size: %s\n", content->m_Size );

							token = colon + 1;
							colon = strstr( token, ":" );
							Dbg_Assert( colon );
							*colon = '\0';
							strcpy( buff, token );
							content->m_NumPieces = atoi( buff );
							Dbg_Printf( "Pieces: %d\n", content->m_NumPieces );

							token = colon + 1;
							colon = strstr( token, ":" );
							Dbg_Assert( colon );
							*colon = '\0';
							strcpy( buff, token );
							content->m_NumGaps = atoi( buff );
							Dbg_Printf( "Gaps: %d\n", content->m_NumGaps );

							token = colon + 1;
							colon = strstr( token, ":" );
							Dbg_Assert( colon );
							*colon = '\0';
							strcpy( buff, token );
							content->m_NumGoals = atoi( buff );
							Dbg_Printf( "Goals: %d\n", content->m_NumGoals );

							token = colon + 1;
							colon = strstr( token, ":" );
							Dbg_Assert( colon );
							*colon = '\0';
							strcpy( buff, token );
							content->m_Theme = atoi( buff );
							Dbg_Printf( "Theme: %d\n", content->m_Theme );

							token = colon + 1;
							colon = strstr( token, ":" );
							Dbg_Assert( colon );
							*colon = '\0';
							strcpy( buff, token );
							content->m_TODScript = atoi( buff );
							Dbg_Printf( "Script: 0x%x\n", content->m_TODScript );
						}

						// Read the content score, if we're looking at tricks
						if( content_man.m_vault_type == CRCD(0x1e26fd3e,"tricks") )
						{
							token = colon + 1;
							colon = strstr( token, ":" );
							Dbg_Assert( colon );
							*colon = '\0';
							strcpy( buff, token );
							content->m_Score = atoi( buff );
							Dbg_Printf( "Score: %d\n", content->m_Score );
						}

						// Read the content sex, if we're looking at skaters
						if( content_man.m_vault_type == CRCD(0x7364ea1,"skaters") )
						{
							token = colon + 1;
							colon = strstr( token, ":" );
							Dbg_Assert( colon );
							*colon = '\0';
							//strcpy( content->m_IsMale, token );
							strcpy( buff, token );
							content->m_IsMale = atoi( buff );
							Dbg_Printf( "IsMale: %d\n", content->m_IsMale );
						}

						// Read the content sex, if we're looking at skaters
						if( content_man.m_vault_type == CRCD(0x38dbe1d0,"goals") )
						{
							token = colon + 1;
							colon = strstr( token, ":" );
							Dbg_Assert( colon );
							*colon = '\0';
							strcpy( buff, token );
							content->m_NumGoals = atoi( buff );
							Dbg_Printf( "Goals: %d\n", content->m_NumGoals );
						}

						token = colon + 1;
						colon = strstr( token, ":" );
						Dbg_Assert( colon );
						*colon = '\0';
						strcpy( content->m_Checksum, token );
						Dbg_Printf( "Checksum: %s\n", content->m_Checksum );

						token = colon + 1;
						strcpy( content->m_Description, token );
						Dbg_Printf( "Desc: %s\n", content->m_Description );
						

						//Dbg_Printf( "////////////////////////\n" );
						//Dbg_Printf( "Name: %s\n", content->m_Name );
						//Dbg_Printf( "Creator: %s\n", content->m_Creator );
						//Dbg_Printf( "Size: %s\n", content->m_Size );
						//Dbg_Printf( "Checksum: %s\n", content->m_Checksum );
						//Dbg_Printf( "Desc: %s\n", content->m_Description );
					} while(( line = strtok( NULL, "\r\n" )));
				}
				
				content_man.m_state = vCONTENT_STATE_IDLE;
				//ghttpCleanup();
				delete [] content_man.m_buffer;

				content_man.m_cur_dir = content_man.m_next_dir_sh.NextItem();
				if(( content_man.m_cur_dir == NULL ) || ( content_man.m_cur_dir->m_Uploads ))
				{
					Script::CStruct* pParams;
					content_man.m_cur_dir = content_man.m_next_dir_sh.FirstItem( content_man.m_directories );

					// We are done. Now display the first category of files
					//Dbg_Printf( "**** Done downloading subdirectory listings\n" );
					content_man.m_http_transfer_task->Remove();
					pParams = new Script::CStruct;
					pParams->AddString( "category", content_man.m_cur_dir->m_Name );
					pParams->AddChecksum( "type", content_man.m_vault_type );
					Script::RunScript( "net_vault_menu_create", pParams );
					delete pParams;
				}
				else
				{
					content_man.GetSubDirListing( content_man.m_cur_dir );
				}
			}
            else
			{
				content_man.m_state = vCONTENT_STATE_IDLE;
				content_man.m_http_transfer_task->Remove();

				//ghttpCleanup();
				delete [] content_man.m_buffer;
				Dbg_Assert( content_man.m_transfer_failed_script );
				Script::RunScript( content_man.m_transfer_failed_script );
			}

			Mem::Manager::sHandle().PopContext();
		}
		else if( content_man.m_state == vCONTENT_STATE_GETTING_UPLOADED_LEVELS )
		{
			char* line, *colon, *token;
			Content* content;

			Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
                                      
			line = strtok( content_man.m_buffer, "\r\n" );
			if( line )
			{
				do
				{
					content = new Content;
					content_man.m_cur_dir->m_content_list.AddToHead( content );

					//Dbg_Printf( "******************************************\n" );
					//Dbg_Printf( "Line: %s\n", line );
					sprintf( content->m_Path, "http://www.thugonline.com/%s/%s", content_man.m_vault_root, line );

					token = line;
					colon = strstr( line, ":" );
					Dbg_Assert( colon );
					*colon = '\0';
					strcpy( content->m_Name, token );

					token = colon + 1;
					colon = strstr( token, ":" );
					Dbg_Assert( colon );
					*colon = '\0';
					strcpy( content->m_Creator, token );

					token = colon + 1;
					colon = strstr( token, ":" );
					Dbg_Assert( colon );
					*colon = '\0';
					strcpy( content->m_Size, token );

					token = colon + 1;
					colon = strstr( token, ":" );
					Dbg_Assert( colon );
					*colon = '\0';
					strcpy( content->m_Checksum, token );

					token = colon + 1;
					strcpy( content->m_Description, token );

					/*Dbg_Printf( "////////////////////////\n" );
					Dbg_Printf( "Name: %s\n", content->m_Name );
					Dbg_Printf( "Creator: %s\n", content->m_Creator );
					Dbg_Printf( "Size: %s\n", content->m_Size );
					Dbg_Printf( "Checksum: %s\n", content->m_Checksum );
					Dbg_Printf( "Desc: %s\n", content->m_Description );
					*/
				} while(( line = strtok( NULL, "\r\n" )));
			}

			content_man.m_state = vCONTENT_STATE_IDLE;
			content_man.m_http_transfer_task->Remove();

			//ghttpCleanup();
			delete [] content_man.m_buffer;

			content_man.DownloadDirectoryListing();

			Mem::Manager::sHandle().PopContext();
		}
		
		//SignalSema( content_man.m_thread_semaphore_id );
	}
}


/*****************************************************************************
**							  Public Functions								**
*****************************************************************************/

void		ContentMan::FreeDirectoryListing( void )
{
	Directory* dir, *next;
	Lst::Search< Directory > sh;

	for( dir = sh.FirstItem( m_directories ); dir; dir = next )
	{
		next = sh.NextItem();
		delete dir;
	}
	ghttpCleanup();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		ContentMan::GetDirListing( char* path )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Thread::PerThreadStruct	net_thread_data;

	sprintf( m_download_path, path );
	//mlp_man->AddLogicTask( *m_http_transfer_task );
	m_percent_complete = 0;
	m_last_comm_time = Tmr::GetTime();

	m_transfer_complete_script = CRCD(0x89990f,"LaunchDownloadCompleteDialog");
	m_transfer_failed_script = CRCD(0x7335ab7f,"LaunchTransferFailedDialog");
	m_file_not_found_script = CRCD(0xcf040665,"LaunchGeneralFileNotFoundDialog");
	m_update_progress_script = CRCD(0x4b85ee64,"update_transfer_progress");

	Dbg_Printf( "**** Downloading path %s\n", m_download_path );

	net_thread_data.m_pEntry = s_threaded_download_directory_list;
	net_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
	net_thread_data.m_pStackBase = gamenet_man->GetNetThreadStack();
	net_thread_data.m_iStackSize = vNET_THREAD_STACK_SIZE;
	net_thread_data.m_utid = 0x152;
	Thread::CreateThread( &net_thread_data );
	gamenet_man->SetNetThreadId( net_thread_data.m_osId );

	//WaitSema( m_thread_semaphore_id );
	StartThread( gamenet_man->GetNetThreadId(), this );
	//SignalSema( m_thread_semaphore_id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		ContentMan::GetSubDirListing( Directory* dir )
{
	char subdir_path[128];

	sprintf( subdir_path, "http://www.thugonline.com/%s/%s/dir_list.txt", m_vault_root, dir->m_Name );
	m_state = vCONTENT_STATE_GETTING_SUB_DIR_LIST;
	m_result = vCONTENT_RESULT_UNDETERMINED;
	GetDirListing( subdir_path );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		ContentMan::FillDirectoryListing( char* dir_text )
{
	char* token;
	Script::CStruct* p_item_params;
	Script::CStruct* p_script_params;

	//Dbg_Printf( "printing buffer\n" );
	//Dbg_Printf( "Buffer: %s\n", dir_text );

	token = strtok( dir_text, "\t" );
	if( token )
	{
		p_item_params = new Script::CStruct;
		p_item_params->AddString( "text", token );
		token = strtok( NULL, "\r\n" );
		Dbg_Assert( token );

		p_item_params->AddChecksum( "pad_choose_script", Script::GenerateCRC("download_selected_file"));

		// create the parameters that are passed to the choose script
		p_script_params= new Script::CStruct;
		p_script_params->AddString( "file", token );	
		p_item_params->AddStructure("pad_choose_params",p_script_params);			

		Script::RunScript( "make_text_sub_menu_item", p_item_params );
		delete p_item_params;
		delete p_script_params;

        while( token )
		{   
			p_item_params = new Script::CStruct;
			
			token = strtok( NULL, "\t" );
			if( token == NULL )
			{
				break;
			}

			p_item_params->AddString( "text", token );
	
			p_item_params->AddChecksum( "pad_choose_script", Script::GenerateCRC("download_selected_file"));
	
			token = strtok( NULL, "\r\n" );
			Dbg_Assert( token );

			// create the parameters that are passed to the choose script
			p_script_params= new Script::CStruct;
			p_script_params->AddString( "file", token );	
			p_item_params->AddStructure("pad_choose_params",p_script_params);			
	
			Script::RunScript( "make_text_sub_menu_item", p_item_params );
			delete p_item_params;
			delete p_script_params;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ContentMan::ContentMan( void )
{
	struct SemaParam params;

	m_state = vCONTENT_STATE_IDLE;
	m_percent_complete = 0;
	m_last_comm_time = 0;
	m_http_transfer_task = new Tsk::Task< ContentMan > ( s_http_transfer_code, *this );

	params.initCount = 1;
	params.maxCount = 10;

	m_thread_semaphore_id = CreateSema( &params );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ContentMan::~ContentMan( void )
{
	DeleteSema( m_thread_semaphore_id );
	delete m_http_transfer_task;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		ContentMan::s_threaded_download_directory_list( ContentMan* content_man )
{
	WaitSema( content_man->m_thread_semaphore_id );

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
	// Register this thread with the sockets API
	sockAPIregthr();

	content_man->m_buffer = new char[100000];
	memset( content_man->m_buffer, 0, 100000 );

	content_man->m_file_request = ghttpGetEx( 	content_man->m_download_path,
												NULL,
												content_man->m_buffer,
												100000,
												NULL,
												GHTTPFalse,
												GHTTPFalse,
												s_progress_callback,
												s_ul_completed_callback,
												content_man );
    
	//delete buffer;

	// Deregister this thread with the sockets API
	sockAPIderegthr();

	//printf( "******** EXITING DOWNLOAD DIRECTORY THREAD\n" );

	Mem::Manager::sHandle().PopContext();

	SignalSema( content_man->m_thread_semaphore_id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	ContentMan::DownloadDirectoryListing( void )
{
	char path[128];
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();

	m_state = vCONTENT_STATE_GETTING_DIR_LIST;
	sprintf( path, "http://www.thugonline.com/%s/dir_list.txt", m_vault_root );
	GetDirListing( path );
	m_result = vCONTENT_RESULT_UNDETERMINED;;
	mlp_man->AddLogicTask( *m_http_transfer_task );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	ContentMan::DownloadFace( char* filename )
{
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Thread::PerThreadStruct	net_thread_data;
	char path[256];
    
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	m_state = vCONTENT_STATE_DOWNLOADING_FACE;
	m_result = vCONTENT_RESULT_UNDETERMINED;
	mlp_man->AddLogicTask( *m_http_transfer_task );
	m_percent_complete = 0;
	m_last_comm_time = Tmr::GetTime();

	m_transfer_complete_script = CRCD(0x1d53d27f,"LaunchFaceDownloadCompleteDialog");
	m_transfer_failed_script = CRCD(0x7335ab7f,"LaunchTransferFailedDialog");
	m_file_not_found_script = CRCD(0x609f7535,"LaunchFileNotFoundDialog");
	m_update_progress_script = CRCD(0x4b85ee64,"update_transfer_progress");

	net_thread_data.m_pEntry = s_threaded_download_face;
	net_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
	net_thread_data.m_pStackBase = gamenet_man->GetNetThreadStack();
	net_thread_data.m_iStackSize = vNET_THREAD_STACK_SIZE;
	net_thread_data.m_utid = 0x152;
	Thread::CreateThread( &net_thread_data );
	gamenet_man->SetNetThreadId( net_thread_data.m_osId );
	
	Dbg_Printf( "In DownloadFace. Path: %s\n", filename );
	WaitSema( m_thread_semaphore_id );
	
	sprintf( path, "http://www.thugonline.com/faces/%s", filename );
	Dbg_Printf( "Downloading %s\n", path );
	StartThread( gamenet_man->GetNetThreadId(), (char*) path );
	SignalSema( m_thread_semaphore_id );

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	ContentMan::ScriptDownloadFace(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	ContentMan* content_man;
	BuddyMan* buddy_man;
	char filename[128];
	const char* key;

	pParams->GetString( "string", &key, Script::ASSERT );
	sprintf( filename, "%s.img.ps2", key );
	Dbg_Printf( "Downloading %s...\n", filename );

	content_man = gamenet_man->mpContentMan;
	buddy_man = gamenet_man->mpBuddyMan;
	buddy_man->GetProfile();
	content_man->DownloadFace( filename );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	ContentMan::ScriptDownloadDirectoryList(Script::CScriptStructure *p_item_params, Script::CScript *pScript)
{
    GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	ContentMan* content_man;
	BuddyMan* buddy_man;

    content_man = gamenet_man->mpContentMan;
	buddy_man = gamenet_man->mpBuddyMan;
	p_item_params->GetChecksum( NONAME, &content_man->m_vault_type , true );
	switch( content_man->m_vault_type )
	{
		case 0x5be5569f:	// parks
			sprintf( content_man->m_vault_root, "levels" );
			break;
		case 0x7364ea1:		// skaters
			sprintf( content_man->m_vault_root, "skaters" );
			break;
		case 0x38dbe1d0:	// goals
			sprintf( content_man->m_vault_root, "goals" );
			break;
		case 0x1e26fd3e:	// tricks
			sprintf( content_man->m_vault_root, "tricks" );
			break;
		default:
			Dbg_Assert( 0 );
			break;
			
	}

	if( buddy_man->GetProfile() == 18549782 )	// Gamespy profile of our level-verifying account
	{
		Directory* dir;
		char path[128];

		Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

		dir = new Directory;
		dir->m_Uploads = true;
		content_man->m_directories.AddToHead( dir );
		strcpy( dir->m_Name, "Uploads" );
		content_man->m_cur_dir = content_man->m_next_dir_sh.FirstItem( content_man->m_directories );

		content_man->m_state = vCONTENT_STATE_GETTING_UPLOADED_LEVELS;
		content_man->m_result = vCONTENT_RESULT_UNDETERMINED;
		sprintf( path, "http://www.thugonline.com/%s/uploads.txt", content_man->m_vault_root );
		content_man->GetDirListing( path );
		mlp_man->AddLogicTask( *content_man->m_http_transfer_task );
		Mem::Manager::sHandle().PopContext();
	}
	else
	{
		content_man->DownloadDirectoryListing();
	}

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	ContentMan::ScriptFreeDirectoryListing(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	ContentMan* content_man;

	content_man = gamenet_man->mpContentMan;
	content_man->FreeDirectoryListing();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		ContentMan::s_threaded_download_file( const char* path )
{
	int max_size;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	ContentMan* content_man;

	content_man = gamenet_man->mpContentMan;
	WaitSema( content_man->m_thread_semaphore_id );

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	max_size = content_man->m_buffer_size;

	content_man->m_buffer = new char[max_size];

	// Register this thread with the sockets API
	sockAPIregthr();

	//Dbg_Printf( "downloading %s, size: %d dl complete: %p\n", path, max_size, content_man->m_dl_complete );
	gamenet_man->mpContentMan->m_file_request = ghttpGetEx( path,
															NULL,
															content_man->m_buffer,
															max_size,
															NULL,
															GHTTPFalse,
															GHTTPFalse,
															s_progress_callback,
															content_man->m_dl_complete,
															content_man );
    
	// Deregister this thread with the sockets API
	sockAPIderegthr();
    
	Mem::Manager::sHandle().PopContext();
	
	SignalSema( gamenet_man->mpContentMan->m_thread_semaphore_id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	ContentMan::s_threaded_download_face( const char* path )
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();

	WaitSema( gamenet_man->mpContentMan->m_thread_semaphore_id );

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	gamenet_man->mpContentMan->m_buffer = new char[vMAX_FACE_SIZE];

	// Register this thread with the sockets API
	sockAPIregthr();

	gamenet_man->mpContentMan->m_file_request = ghttpGetEx( path,
															NULL,
															gamenet_man->mpContentMan->m_buffer,
															vMAX_FACE_SIZE,
															NULL,
															GHTTPFalse,
															GHTTPFalse,
															s_progress_callback,
															s_face_dl_completed_callback,
															gamenet_man->mpContentMan );
    
	// Deregister this thread with the sockets API
	sockAPIderegthr();
    
	Mem::Manager::sHandle().PopContext();

	SignalSema( gamenet_man->mpContentMan->m_thread_semaphore_id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	ContentMan::SetPercentComplete( int percent_complete )
{
	m_percent_complete = percent_complete;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	ContentMan::SetResult( int result )
{
	m_result = result;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	ContentMan::SetError( int error )
{
	m_error = error;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	ContentMan::DownloadFile( DownloadContext* context )
{
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	ContentMan* content_man;
	Thread::PerThreadStruct	net_thread_data;
    
	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());

	Dbg_Printf( "Downloading %s\n", context->m_path );
	
	content_man = gamenet_man->mpContentMan;
	content_man->m_state = vCONTENT_STATE_DOWNLOADING;
	content_man->m_result = vCONTENT_RESULT_UNDETERMINED;
	mlp_man->AddLogicTask( *content_man->m_http_transfer_task );
	content_man->m_percent_complete = 0;
	content_man->m_last_comm_time = Tmr::GetTime();
	
	content_man->m_buffer_size = context->m_max_size;
	content_man->m_transfer_failed_script = context->m_transfer_failed_script;
	content_man->m_transfer_complete_script = context->m_transfer_complete_script;
	content_man->m_update_progress_script = context->m_update_progress_script;
	content_man->m_file_not_found_script = context->m_file_not_found_script;
	content_man->m_dl_complete = context->m_dl_complete;

	net_thread_data.m_pEntry = s_threaded_download_file;
	net_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
	net_thread_data.m_pStackBase = gamenet_man->GetNetThreadStack();
	net_thread_data.m_iStackSize = vNET_THREAD_STACK_SIZE;
	net_thread_data.m_utid = 0x152;
	Thread::CreateThread( &net_thread_data );
	gamenet_man->SetNetThreadId( net_thread_data.m_osId );
	
	WaitSema( content_man->m_thread_semaphore_id );
	StartThread( gamenet_man->GetNetThreadId(), context->m_path );
	SignalSema( content_man->m_thread_semaphore_id );

	Mem::Manager::sHandle().PopContext();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	ContentMan::ScriptDownloadFile(Script::CScriptStructure *p_item_params, Script::CScript *pScript)
{
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	ContentMan* content_man;
	const char* path;
	DownloadContext context;
	
	p_item_params->GetString( "file", &path );

	content_man = gamenet_man->mpContentMan;
	
	context.m_transfer_complete_script = CRCD(0x89990f,"LaunchDownloadCompleteDialog");
	context.m_transfer_failed_script = CRCD(0x7335ab7f,"LaunchTransferFailedDialog");
	context.m_file_not_found_script = CRCD(0xcf040665,"LaunchGeneralFileNotFoundDialog");
	context.m_update_progress_script = CRCD(0x4b85ee64,"update_transfer_progress");
	context.m_max_size = vMAX_FILE_SIZE;
	sprintf( context.m_path, path );
	context.m_dl_complete = s_dl_completed_callback;
	content_man->DownloadFile( &context );
	
	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************

static char* s_fix_name( char* filename )
{
	char* ch = filename;
	
	while( *ch )
	{
		if(( *ch >= '0' ) && ( *ch <= '9' ))
		{
			ch++;
			continue;
		}

		if(( *ch >= 'A' ) && ( *ch <= 'Z' ))
		{
			ch++;
			continue;
		}

		if(( *ch >= 'a' ) && ( *ch <= 'z' ))
		{
			ch++;
			continue;
		}

		if(( *ch == '-' ) || ( *ch =='_' ))
		{
			ch++;
			continue;
		}

		*ch = '_';
		ch++;
	}

	return filename;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		ContentMan::s_threaded_upload_file( ContentMan* content_man )
{
	GHTTPPost post;
	char *file_buffer; 
	char filename[64], desc[1024], str[64];
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Prefs::Preferences* pPreferences;
	Script::CStruct* pStructure;
	const char* network_id;
	int filesize;
	uint32 checksum;

	pPreferences = gamenet_man->GetNetworkPreferences();
	pStructure = pPreferences->GetPreference( Script::GenerateCRC("network_id") );
	pStructure->GetText( "ui_string", &network_id, true );

	WaitSema( content_man->m_thread_semaphore_id );
	sockAPIregthr();

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetBottomUpHeap());
	post = ghttpNewPost();
	switch( content_man->m_vault_type )
	{
		case 0x5be5569f:	// parks
		{
			filesize = Script::CalculateBufferSize( content_man->m_upload_struct );
			file_buffer = new char[filesize];
			Script::WriteToBuffer( content_man->m_upload_struct, (uint8*) file_buffer, filesize );
			checksum = Crc::GenerateCRC( file_buffer, filesize );
		
			strcpy( filename, content_man->m_upload_filename );
			ghttpPostAddString( post, "player", network_id );
			sprintf( str, "%d", content_man->m_Width );
			ghttpPostAddString( post, "width", str );
			sprintf( str, "%d", content_man->m_Height );
			ghttpPostAddString( post, "height", str );
			sprintf( str, "%d", checksum );
			ghttpPostAddString( post, "checksum", str );
			
			sprintf( str, "%d", content_man->m_NumPieces );
			ghttpPostAddString( post, "num_pieces", str );
			sprintf( str, "%d", content_man->m_NumGaps );
			ghttpPostAddString( post, "num_gaps", str );
			sprintf( str, "%d", content_man->m_NumGoals );
			ghttpPostAddString( post, "num_goals", str );
			sprintf( str, "%d", content_man->m_Theme );
			ghttpPostAddString( post, "theme", str );
			sprintf( str, "0x%x", content_man->m_TODScript );
			ghttpPostAddString( post, "tod_script", str );
			break;
		}

		case 0x7364ea1:		// skaters
		{
			ghttpPostAddString( post, "filetype", "cas" );
			sprintf( str, "%d", content_man->m_IsMale );
			ghttpPostAddString( post, "is_male", str );
			
			filesize = Script::CalculateBufferSize( content_man->m_upload_struct );
			file_buffer = new char[filesize];
			Script::WriteToBuffer( content_man->m_upload_struct, (uint8*) file_buffer, filesize );
			checksum = Crc::GenerateCRC( file_buffer, filesize );
			strcpy( filename, content_man->m_upload_filename );
			ghttpPostAddString( post, "player", network_id );
			sprintf( str, "%d", checksum );
			ghttpPostAddString( post, "checksum", str );
				
			Dbg_Printf( "Uploading file: %s\n", filename );

			break;
		}

		case 0x38dbe1d0:	// goals
		{
			ghttpPostAddString( post, "filetype", "cag" );
			sprintf( str, "%d", content_man->m_NumGoals );
			ghttpPostAddString( post, "num_goals", str );

			filesize = Script::CalculateBufferSize( content_man->m_upload_struct );
			file_buffer = new char[filesize];
			Script::WriteToBuffer( content_man->m_upload_struct, (uint8*) file_buffer, filesize );
			checksum = Crc::GenerateCRC( file_buffer, filesize );
			strcpy( filename, content_man->m_upload_filename );
			ghttpPostAddString( post, "player", network_id );
			sprintf( str, "%d", checksum );
			ghttpPostAddString( post, "checksum", str );
			break;
		}
		case 0x1e26fd3e:	// tricks
		{
			ghttpPostAddString( post, "filetype", "cat" );
			sprintf( str, "%d", content_man->m_Score );
			ghttpPostAddString( post, "score", str );

			filesize = Script::CalculateBufferSize( content_man->m_upload_struct );
			file_buffer = new char[filesize];
			Script::WriteToBuffer( content_man->m_upload_struct, (uint8*) file_buffer, filesize );
			checksum = Crc::GenerateCRC( file_buffer, filesize );
			strcpy( filename, content_man->m_upload_filename );
			ghttpPostAddString( post, "player", network_id );
			sprintf( str, "%d", checksum );
			ghttpPostAddString( post, "checksum", str );
			break;
		}
		default:
			file_buffer = NULL;
			filesize = 0;
			Dbg_Assert( 0 );
			break;
	}
		
	content_man->m_buffer = file_buffer;
	content_man->m_buffer_size = filesize;
	ghttpPostAddFileFromMemory( post, "filename", file_buffer, filesize, filename, NULL );

	sprintf( desc, "%s", content_man->m_upload_description );
	ghttpPostAddString( post, "description", desc );

	ghttpPostAddString( post, "uniqueid", GOAGetUniqueID() );
	switch( content_man->m_vault_type )
	{
		case 0x5be5569f:	// parks
			ghttpPostAddString( post, "filetype", "cap" );
			break;
		case 0x7364ea1:		// skaters
			ghttpPostAddString( post, "filetype", "cas" );
			break;
		case 0x38dbe1d0:	// goals
			ghttpPostAddString( post, "filetype", "cag" );
			break;
		case 0x1e26fd3e:	// tricks
			ghttpPostAddString( post, "filetype", "cat" );
			break;
		default:
			Dbg_Assert( 0 );
			break;
	}
	
	// debugger log
	ghttpPostAddString( post, "dblog", "1" );

	content_man->m_file_request = ghttpPostEx( "http://www.thugonline.com/cgi-bin/upload2.pl", NULL, post, GHTTPFalse, GHTTPFalse, s_progress_callback, s_ul_completed_callback, content_man );	
	
    
    // Deregister this thread with the sockets API
	sockAPIderegthr();
	Mem::Manager::sHandle().PopContext();

	SignalSema( content_man->m_thread_semaphore_id );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	ContentMan::ScriptUploadFile(Script::CScriptStructure *p_item_params, Script::CScript *pScript)
{
	Mlp::Manager * mlp_man = Mlp::Manager::Instance();
	ContentMan* content_man;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Thread::PerThreadStruct	net_thread_data;
	const char* desc, *filename;

	content_man = gamenet_man->mpContentMan;
	content_man->m_state = vCONTENT_STATE_UPLOADING;
	content_man->m_result = vCONTENT_RESULT_UNDETERMINED;
	mlp_man->AddLogicTask( *content_man->m_http_transfer_task );
	content_man->m_percent_complete = 0;
	content_man->m_last_comm_time = Tmr::GetTime();
	content_man->m_transfer_failed_script = CRCD(0x7335ab7f,"LaunchTransferFailedDialog");

	//Script::PrintContents( p_item_params );
	p_item_params->GetChecksum( "type", &content_man->m_vault_type , true );
	p_item_params->GetStructure( "data", &content_man->m_upload_struct );
	p_item_params->GetString( "description", &desc, Script::ASSERT );
	p_item_params->GetString( "filename", &filename, Script::ASSERT );
	strcpy( content_man->m_upload_filename, filename );
	strcpy( content_man->m_upload_description, desc );
	
	p_item_params->GetInteger( "score", &content_man->m_Score, false );
	p_item_params->GetInteger( "width", &content_man->m_Width, false );
	p_item_params->GetInteger( "length", &content_man->m_Height, false );
	p_item_params->GetInteger( "num_pieces", &content_man->m_NumPieces, false );
	p_item_params->GetInteger( "num_gaps", &content_man->m_NumGaps, false );
	p_item_params->GetInteger( "num_goals", &content_man->m_NumGoals, false );
	p_item_params->GetInteger( "theme", &content_man->m_Theme, false );
	p_item_params->GetChecksum( "tod_script", &content_man->m_TODScript, false );
	p_item_params->GetInteger( "is_male", &content_man->m_IsMale, false );
    
	net_thread_data.m_pEntry = s_threaded_upload_file;
	net_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
	net_thread_data.m_pStackBase = gamenet_man->GetNetThreadStack();
	net_thread_data.m_iStackSize = vNET_THREAD_STACK_SIZE;
	net_thread_data.m_utid = 0x152;
	Thread::CreateThread( &net_thread_data );
	gamenet_man->SetNetThreadId( net_thread_data.m_osId );

	WaitSema( content_man->m_thread_semaphore_id );
	StartThread( gamenet_man->GetNetThreadId(), content_man );
	SignalSema( content_man->m_thread_semaphore_id );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	ContentMan::ScriptFillVaultMenu(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	ContentMan* content_man;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Lst::Search< Content > sh;
	Content* content;

	content_man = gamenet_man->mpContentMan;

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().InternetTopDownHeap());

	for( content = sh.FirstItem( content_man->m_cur_dir->m_content_list ); content; content = sh.NextItem())
	{
		Script::CStruct* pParams, *pChooseParams;

		pParams = new Script::CStruct;

		pParams->AddString( "name", content->m_Name );
		pParams->AddString( "creator", content->m_Creator );
		pParams->AddString( "size", content->m_Size );
		pParams->AddInteger( "score", content->m_Score );
		pParams->AddInteger( "num_pieces", content->m_NumPieces );
		pParams->AddInteger( "num_gaps", content->m_NumGaps );
		pParams->AddInteger( "width", content->m_Width );
		pParams->AddInteger( "height", content->m_Height );
		pParams->AddInteger( "num_goals", content->m_NumGoals );
		pParams->AddInteger( "theme", content->m_Theme );
		pParams->AddChecksum( "tod_script", content->m_TODScript );
		pParams->AddInteger( "is_male", content->m_IsMale );
		pParams->AddString( "description", content->m_Description );

		pChooseParams = new Script::CStruct;

		pChooseParams->AddString( "file", content->m_Path );
		
		pParams->AddStructure( "pad_choose_params", pChooseParams );
		switch( content_man->m_vault_type )
		{
			case 0x5be5569f:	// parks
				Script::RunScript( "net_vault_menu_add_park", pParams );
				break;
			case 0x7364ea1:		// skaters
				Script::RunScript( "net_vault_menu_add_skater", pParams );
				break;
			case 0x38dbe1d0:	// goals
				Script::RunScript( "net_vault_menu_add_goal", pParams );
				break;
			case 0x1e26fd3e:	// tricks
				Script::RunScript( "net_vault_menu_add_trick", pParams );
				break;
			default:
				Dbg_Assert( 0 );
				break;
		}
		

		delete pParams;
		delete pChooseParams;
	}

	Mem::Manager::sHandle().PopContext();

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	ContentMan::ScriptNextVaultCategory(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	ContentMan* content_man;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Script::CStruct* passback_params;

	content_man = gamenet_man->mpContentMan;
	content_man->m_cur_dir = content_man->m_next_dir_sh.NextItem();
	if( content_man->m_cur_dir == NULL )
	{
		content_man->m_cur_dir = content_man->m_next_dir_sh.FirstItem( content_man->m_directories );
	}

	passback_params = pScript->GetParams();
	passback_params->AddString( "category", content_man->m_cur_dir->m_Name );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	ContentMan::ScriptPrevVaultCategory(Script::CScriptStructure *pParams, Script::CScript *pScript)
{
	ContentMan* content_man;
	GameNet::Manager * gamenet_man = GameNet::Manager::Instance();
	Script::CStruct* passback_params;

	content_man = gamenet_man->mpContentMan;
	content_man->m_cur_dir = content_man->m_next_dir_sh.PrevItem();
	if( content_man->m_cur_dir == NULL )
	{
		content_man->m_cur_dir = content_man->m_next_dir_sh.LastItem( content_man->m_directories );
	}

	passback_params = pScript->GetParams();
	passback_params->AddString( "category", content_man->m_cur_dir->m_Name );

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Directory::Directory( void )
: Lst::Node< Directory > ( this )
{
	m_Uploads = false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Directory::~Directory( void )
{
	Content* content, *next;
	Lst::Search< Content > sh;

	for( content = sh.FirstItem( m_content_list ); content; content = next )
	{
		next = sh.NextItem();
		delete content;
	}

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Content::Content( void )
: Lst::Node< Content > ( this )
{
	m_Path[0] = '\0';
	m_Name[0] = '\0';
	m_Creator[0] = '\0';
	m_Size[0] = '\0';
	m_Checksum[0] = '\0';
	m_Description[0] = '\0';
	m_Score = 0;
	m_NumPieces = 0;
	m_NumGaps = 0;
	m_NumGoals = 0;
	m_Theme = 0;
	m_TODScript = 0;
	m_IsMale = 0;
	m_Width = 0;
	m_Height = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace GameNet
