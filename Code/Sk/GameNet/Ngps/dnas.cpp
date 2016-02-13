#include <stdio.h>
#include <libgraph.h>
#include <dnas/dnas2.h>
#include <sys/file/filesys.h>
#include <sys/config/config.h>
#include <gamenet/gamenet.h>
#include <gel/scripting/struct.h>
#include <gel/scripting/script.h>
#include <gel/scripting/symboltable.h>
#include <libcdvd.h>

#define WAIT_VSYNC	0x1111
#define vAUTH_TIMEOUT	30

static	int	s_extra_time = 0;

//#define __DNAS_DEBUG__

static int s_get_thread_priority( void )
{
	struct ThreadParam info;
	// Fill info with zeros to be sure the following call is doing
	// something to info
	memset(&info,0,sizeof(info));
	// return value not defined in 2.0 PDF EE lib ref
	ReferThreadStatus( GetThreadId(), &info );
	return ( info.currentPriority );
}

static int netstart_dnas(int proxy_state, const char *proxy_host, u_short proxy_port, int timeout, sceDNAS2Status_t& status) 
{
	int  ret, error = 0;
	sceDNAS2TitleAuthInfo_t auth_info;
	sceDNAS2Status_t prev_status;
	sceDNAS2TimeoutInfo_t timeout_info;
	int /*fd, */proxy_state_check = 0;
	char* auth_data;
	char proxy_host_check[512];
	u_short proxy_port_check;
	unsigned char* phrase;
	//int vsync_handler_id;
	int orig_priority;

    unsigned char us_english_pass_phrase[] = { 0xfb, 0x31, 0xce, 0xf7, 0x2a, 0x57, 0x2f, 0xf4 };
	unsigned char uk_english_pass_phrase[] = { 0xea, 0x67, 0x94, 0x2c, 0x37, 0xd1, 0xfc, 0x8f };
	unsigned char spanish_pass_phrase[] = { 0xb5, 0x05, 0xcb, 0x0b, 0xe5, 0x20, 0x8e, 0x38 };
    unsigned char german_pass_phrase[] = { 0x91, 0xe7, 0x87, 0x5c, 0xc5, 0x3f, 0x70, 0x4c };
    unsigned char french_pass_phrase[] = { 0x15, 0xb3, 0x8d, 0x7f, 0x2d, 0x81, 0xbe, 0x05 };
    unsigned char italian_pass_phrase[] = { 0x88, 0xe9, 0xfa, 0x1e, 0xb4, 0x3d, 0x88, 0x3c };

	phrase = NULL;

	auth_data = new char[1024*64];

		/* Initialize ID module for DNAS */
	void *p_file;
	 
	p_file = NULL;
	if( Config::gLanguage == Config::LANGUAGE_ENGLISH )
	{
		if( Config::gDisplayType == Config::DISPLAY_TYPE_NTSC )
		{
#ifdef __DNAS_DEBUG__
			scePrintf( "Auth File: dnas/SLUS20731_client.dat\n" );
#endif
			p_file = File::Open( "dnas/SLUS20731_client.dat", "rb" );
			phrase = us_english_pass_phrase;
		}
		else
		{
#ifdef __DNAS_DEBUG__
			scePrintf( "Auth File: dnas/SLES51848_client.dat\n" );
#endif
			p_file = File::Open( "dnas/SLES51848_client.dat", "rb" );
			phrase = uk_english_pass_phrase;
		}
	}
	else if( Config::gLanguage == Config::LANGUAGE_FRENCH )
	{
#ifdef __DNAS_DEBUG__
		scePrintf( "Auth File: dnas/SLES51851_client.dat\n" );
#endif
		p_file = File::Open( "dnas/SLES51851_client.dat", "rb" );
		phrase = french_pass_phrase;
	}
	else if( Config::gLanguage == Config::LANGUAGE_GERMAN )
	{
#ifdef __DNAS_DEBUG__
		scePrintf( "Auth File: dnas/SLES51852_client.dat\n" );
#endif
		p_file = File::Open( "dnas/SLES51852_client.dat", "rb" );
		phrase = german_pass_phrase;
	}
	else if( Config::gLanguage == Config::LANGUAGE_ITALIAN )
	{
#ifdef __DNAS_DEBUG__
		scePrintf( "Auth File: dnas/SLES51853_client.dat\n" );
#endif
		p_file = File::Open( "dnas/SLES51853_client.dat", "rb" );
		phrase = italian_pass_phrase;
	}
	else if( Config::gLanguage == Config::LANGUAGE_SPANISH )
	{
#ifdef __DNAS_DEBUG__
		scePrintf( "Auth File: dnas/SLES51854_client.dat\n" );
#endif
		p_file = File::Open( "dnas/SLES51854_client.dat", "rb" );
		phrase = spanish_pass_phrase;
	}
	else
	{
#ifdef __DNAS_DEBUG__
		scePrintf( "Auth File: NONE!\n" );
#endif
	}

	Dbg_Assert( p_file != NULL );
	Dbg_Assert( phrase != NULL );

#ifdef __DNAS_DEBUG__
	scePrintf( "p_file: 0x%x  phrase: 0x%x\n", p_file, phrase );
#endif

	//fd = sceOpen("host0:../../authfile/auth.dat", SCE_RDONLY);
	Dbg_Printf( "install_dnas(1)\n" );

	File::Read( auth_data, 1, 1024 * 64, p_file );
	//sceRead(fd, auth_data, sizeof(auth_data));

	Dbg_Printf( "install_dnas(2)\n" );
	File::Close( p_file );

	orig_priority = s_get_thread_priority();
	ChangeThreadPriority( GetThreadId(), 49 );

	memset(&status, 0, sizeof(sceDNAS2Status_t));
	memset(&prev_status, 0, sizeof(sceDNAS2Status_t));
	memset(&auth_info, 0, sizeof(sceDNAS2TitleAuthInfo_t));
	if( timeout > 0 )
	{
		memset(&timeout_info, 0, sizeof(sceDNAS2TimeoutInfo_t));
		timeout_info.timeout = timeout;
		timeout_info.priority = 48;
	}
	auth_info.line_type = sceDNAS2_T_INET;
	auth_info.auth_data.ptr = auth_data;
	auth_info.auth_data.size = 1024 * 64;//sizeof(auth_data);
	auth_info.pass_phrase.ptr = phrase;
	auth_info.pass_phrase.size = sizeof(phrase);
#ifdef __DNAS_DEBUG__
	scePrintf("before sceDNAS2Init()\n");
#endif

	if(timeout > 0)
	{
		ret = sceDNAS2InitNoHDD(&auth_info, 48, 0, 0, &timeout_info);
	}
	else
	{
		ret = sceDNAS2InitNoHDD(&auth_info, 48, 0, 0, NULL);
	}
	/* NOTE */
	/* sceDNAS2InitNoHDD() with Debug flag should never be used in the final release, but for test program only */
#ifdef __DNAS_DEBUG__
	scePrintf("return value of sceDNAS2Init(): %d\n", ret);
#endif
	if(ret < 0)
	{
		sceDNAS2GetStatus( &status );
		
		error = ret;
		goto error;
	}

	/*set proxy parameter*/
	if(proxy_state)
	{
		ret = sceDNAS2SetProxy(1, proxy_host, proxy_port);
#ifdef __DNAS_DEBUG__
		scePrintf("return value of sceDNAS2SetProxy = %d\n", ret);
#endif
		if(ret < 0)
		{
			sceDNAS2GetStatus( &status );
			error = ret;
			goto error;
		}
	}

	/*get proxy parameter*/
	if(proxy_state)
	{
		ret = sceDNAS2GetProxy(&proxy_state_check, proxy_host_check, sizeof(proxy_host_check), &proxy_port_check);
#ifdef __DNAS_DEBUG__
		scePrintf("return value of sceDNASGetProxy = %d\n", ret);
#endif
		if(ret < 0)
		{
			sceDNAS2GetStatus( &status );
			error = ret;
			goto error;
		}
#ifdef __DNAS_DEBUG__
		if(proxy_state_check)
		{
			scePrintf("Proxy Info : Server = %s Port = %d\n", proxy_host_check, proxy_port_check);
		}
		else
		{
			scePrintf("Proxy Info : no use \n");
		}
#endif
	}

	ret = sceDNAS2AuthNetStart();
#ifdef __DNAS_DEBUG__
	scePrintf("ret of AuthNetStart() = %d\n", ret);
#endif
	if(ret < 0)
	{
		sceDNAS2GetStatus( &status );
		error = ret;
		goto error;
	}

	while(1)
	{
		if((ret = sceDNAS2GetStatus(&status)) < 0)
		{
#ifdef __DNAS_DEBUG__
			scePrintf("ret of sceDNAS2GetStatus() = %d\n", ret);
#endif
			error = ret;
			goto error;
		}
		if((ret = memcmp(&prev_status, &status, sizeof(sceDNAS2Status_t))) != 0)
		{
			/* status changed */
#ifdef __DNAS_DEBUG__
			scePrintf("code: %d, sub_code: %d, progress: %d\n",
				status.code, status.sub_code, status.progress);
#endif
			if(status.code == sceDNAS2_S_END)
			{
#ifdef __DNAS_DEBUG__
				scePrintf("subcode:type %d, inst_result %d\n",
					sceDNAS2Status_SubCode_TYPE(status),
					sceDNAS2Status_SubCode_INST_RESULT(status));
#endif
				memcpy(&prev_status, &status, sizeof(sceDNAS2Status_t));
				break;
			}
			else if(status.code == sceDNAS2_S_NG || status.code == sceDNAS2_S_COM_ERROR)
			{
				error = -1;
				break;
			}
		}
		memcpy(&prev_status, &status, sizeof(sceDNAS2Status_t));
	}

error:
#ifdef __DNAS_DEBUG__
	if( error < 0 )
	{
		scePrintf("*********** DNAS failed. Error code : %d**********\n", error );
	}
	else
	{
		scePrintf("*********** DNAS succeeded.!!!! *************\n");
	}
#endif
	ret = sceDNAS2Shutdown();
#ifdef __DNAS_DEBUG__
	scePrintf("ret of sceDNAS2Shutdown() is %d\n", ret);
#endif
	ChangeThreadPriority( GetThreadId(), orig_priority );
	delete [] auth_data;

	return error;
}

static	uint32	s_get_error_desc( int code )
{
	switch( code )
	{
		case sceDNAS2_SS_SERVER_BUSY:
			return CRCD(0xf9814711,"net_auth_error_server_busy");
			break;
		case sceDNAS2_SS_BEFORE_SERVICE:
			return CRCD(0x87da162f,"net_auth_error_before_service");
			break;
		case sceDNAS2_SS_OUT_OF_SERVICE:
			return CRCD(0xa7b3ce4c,"net_auth_error_out_of_service");
			break;
		case sceDNAS2_SS_END_OF_SERVICE:
			return CRCD(0x2a52e052,"net_auth_error_end_of_service");
			break;
		case sceDNAS2_SS_SESSION_TIME_OUT:
			return CRCD(0x6b58d871,"net_auth_error_time_out");
			break;
		case sceDNAS2_SS_INVALID_SERVER:
			return CRCD(0x3472f158,"net_auth_error_invalid_server");
			break;
		case sceDNAS2_SS_INTERNAL_ERROR:
			return CRCD(0x3c0c381,"net_auth_error_internal");
			break;
		case sceDNAS2_SS_EXTERNAL_ERROR:
			return CRCD(0x50617f3a,"net_auth_error_external");
			break;
		case sceDNAS2_SS_ID_NOUSE:
		case sceDNAS2_SS_ID_NOT_JOIN_TO_CAT:
			return CRCD(0xe8177760,"net_auth_error_unique");
			break;

		case sceDNAS2_SS_DL_NODATA:
		case sceDNAS2_SS_DL_BEFORE_SERVICE:
		case sceDNAS2_SS_DL_OUT_OF_SERVICE:
		case sceDNAS2_SS_DL_NOT_UPDATED:
			return CRCD(0x70295cf2,"net_auth_error_download");
			break;

		case sceDNAS2_SS_INVALID_PS2:
		case sceDNAS2_SS_INVALID_HDD_BINDING:
			return CRCD(0x448c00ef,"net_auth_error_machine");
			break;

		case sceDNAS2_SS_INVALID_MEDIA:
		case sceDNAS2_SS_INVALID_AUTHDATA:
			return CRCD(0x736c330,"net_auth_error_disc");
			break;

		case sceDNAS2_ERR_GLUE_ABORT:
		case sceDNAS2_ERR_NET_PROXY:
		case sceDNAS2_ERR_NET_SSL:
		case sceDNAS2_ERR_NET_DNS_HOST_NOT_FOUND:
		case sceDNAS2_ERR_NET_DNS_TRY_AGAIN:
		case sceDNAS2_ERR_NET_DNS_NO_RECOVERY:
		case sceDNAS2_ERR_NET_DNS_NO_DATA:
		case sceDNAS2_ERR_NET_DNS_OTHERS:
		case sceDNAS2_ERR_NET_EISCONN:
		case sceDNAS2_ERR_NET_ETIMEOUT:
		case sceDNAS2_ERR_NET_TIMEOUT:
		case sceDNAS2_ERR_NET_ECONNREFUSED:
		case sceDNAS2_ERR_NET_ENETUNREACH:
		case sceDNAS2_ERR_NET_ENOTCONN:
		case sceDNAS2_ERR_NET_ENOBUFS:
		case sceDNAS2_ERR_NET_EMFILE:
		case sceDNAS2_ERR_NET_EBADF:
		case sceDNAS2_ERR_NET_EINVAL:
		case sceDNAS2_ERR_NET_OTHERS:
			return CRCD(0x310d58d7,"net_auth_error_network");
			break;
		default:
			return CRCD(0xc7c67eb8,"net_auth_error_generic");
			break;
	}
}

static	uint32	s_get_error_footer( int code )
{
	switch( code )
	{
		case sceDNAS2_SS_BEFORE_SERVICE:
		case sceDNAS2_SS_OUT_OF_SERVICE:
		case sceDNAS2_SS_END_OF_SERVICE:
		case sceDNAS2_SS_INVALID_SERVER:
		case sceDNAS2_SS_INTERNAL_ERROR:
		case sceDNAS2_SS_EXTERNAL_ERROR:
		case sceDNAS2_SS_INVALID_PS2:
		case sceDNAS2_SS_INVALID_HDD_BINDING:
		case sceDNAS2_SS_INVALID_MEDIA:
		case sceDNAS2_SS_INVALID_AUTHDATA:
			return CRCD(0x16ff6a40,"net_auth_footer_contact");
		case sceDNAS2_SS_SERVER_BUSY:
		case sceDNAS2_SS_SESSION_TIME_OUT:
		case sceDNAS2_SS_ID_NOUSE:
		case sceDNAS2_SS_ID_NOT_JOIN_TO_CAT:
		case sceDNAS2_SS_DL_NODATA:
		case sceDNAS2_SS_DL_BEFORE_SERVICE:
		case sceDNAS2_SS_DL_OUT_OF_SERVICE:
		case sceDNAS2_SS_DL_NOT_UPDATED:
			return CRCD(0x8c671de9,"net_auth_footer_empty");
		case sceDNAS2_ERR_GLUE_ABORT:
		case sceDNAS2_ERR_NET_PROXY:
		case sceDNAS2_ERR_NET_SSL:
		case sceDNAS2_ERR_NET_DNS_HOST_NOT_FOUND:
		case sceDNAS2_ERR_NET_DNS_TRY_AGAIN:
		case sceDNAS2_ERR_NET_DNS_NO_RECOVERY:
		case sceDNAS2_ERR_NET_DNS_NO_DATA:
		case sceDNAS2_ERR_NET_DNS_OTHERS:
		case sceDNAS2_ERR_NET_EISCONN:
		case sceDNAS2_ERR_NET_ETIMEOUT:
		case sceDNAS2_ERR_NET_TIMEOUT:
		case sceDNAS2_ERR_NET_ECONNREFUSED:
		case sceDNAS2_ERR_NET_ENETUNREACH:
		case sceDNAS2_ERR_NET_ENOTCONN:
		case sceDNAS2_ERR_NET_ENOBUFS:
		case sceDNAS2_ERR_NET_EMFILE:
		case sceDNAS2_ERR_NET_EBADF:
		case sceDNAS2_ERR_NET_EINVAL:
		case sceDNAS2_ERR_NET_OTHERS:
			return CRCD(0x3a190bc4,"net_auth_footer_network");
		default:
			return CRCD(0x16ff6a40,"net_auth_footer_contact");
	}
}

static	uint32	s_get_error_footer_pal( int code )
{
	switch( code )
	{
		case sceDNAS2_SS_SERVER_BUSY:
			return CRCD(0x8c671de9,"net_auth_footer_empty");
		case sceDNAS2_SS_BEFORE_SERVICE:
		case sceDNAS2_SS_OUT_OF_SERVICE:
			return CRCD(0x99b34c96,"net_auth_footer_service_pal");
		case sceDNAS2_SS_END_OF_SERVICE:
			return CRCD(0x1a95350b,"net_auth_footer_central_pal");
		case sceDNAS2_SS_SESSION_TIME_OUT:
		case sceDNAS2_SS_INVALID_SERVER:
		case sceDNAS2_SS_INTERNAL_ERROR:
		case sceDNAS2_SS_EXTERNAL_ERROR:
			return CRCD(0x3e6e83d0,"net_auth_footer_cont_customer_pal");
		case sceDNAS2_SS_ID_NOUSE:
		case sceDNAS2_SS_ID_NOT_JOIN_TO_CAT:
		case sceDNAS2_SS_DL_NODATA:
		case sceDNAS2_SS_DL_BEFORE_SERVICE:
		case sceDNAS2_SS_DL_OUT_OF_SERVICE:
		case sceDNAS2_SS_DL_NOT_UPDATED:
			return CRCD(0x8c671de9,"net_auth_footer_empty");
		case sceDNAS2_SS_INVALID_PS2:
		case sceDNAS2_SS_INVALID_MEDIA:
		case sceDNAS2_SS_INVALID_HDD_BINDING:
			return CRCD(0xc13153cc,"net_auth_footer_customer_pal");
		case sceDNAS2_SS_INVALID_AUTHDATA:
			return CRCD(0x85cb1e0c,"net_auth_footer_clean_pal");
		case sceDNAS2_ERR_GLUE_ABORT:
		case sceDNAS2_ERR_NET_PROXY:
		case sceDNAS2_ERR_NET_SSL:
		case sceDNAS2_ERR_NET_DNS_HOST_NOT_FOUND:
		case sceDNAS2_ERR_NET_DNS_TRY_AGAIN:
		case sceDNAS2_ERR_NET_DNS_NO_RECOVERY:
		case sceDNAS2_ERR_NET_DNS_NO_DATA:
		case sceDNAS2_ERR_NET_DNS_OTHERS:
		case sceDNAS2_ERR_NET_EISCONN:
		case sceDNAS2_ERR_NET_ETIMEOUT:
		case sceDNAS2_ERR_NET_TIMEOUT:
		case sceDNAS2_ERR_NET_ECONNREFUSED:
		case sceDNAS2_ERR_NET_ENETUNREACH:
		case sceDNAS2_ERR_NET_ENOTCONN:
		case sceDNAS2_ERR_NET_ENOBUFS:
		case sceDNAS2_ERR_NET_EMFILE:
		case sceDNAS2_ERR_NET_EBADF:
		case sceDNAS2_ERR_NET_EINVAL:
		case sceDNAS2_ERR_NET_OTHERS:
			return CRCD(0x570be62b,"net_auth_footer_network_pal");
			break;
		default:
			return CRCD(0x1a95350b,"net_auth_footer_central_pal");
			break;
	}
}


extern char _dnas_start[];
extern char _dnas_size[];

bool 		GameNet::Manager::ScriptAuthenticateClient( Script::CStruct* pParams, Script::CScript* pScript )
{
	sceDNAS2Status_t status;
	int result;
	void *p_file;
	char* memory;
	
	// Hardware will be flagged as Config::HARDWARE_PS2_DEVSYSTEM if we run a 
	// .elf file.  This is probably different from what we are running on
	// the cd, so just return true, and don't actually perform the DNAS check
	// (Note, we must be running off a CD, as the check is only performed on CD burns)										  
	// If we actually BOOT the cd on the dev system, it will think it's
	// on a regular PS2, and the DNAS test will be performed
	if 	( Config::CD() && Config::GetHardware() == Config::HARDWARE_PS2_DEVSYSTEM ) 
	{
		Script::RunScript( CRCD(0xbd54c02b,"connected_to_internet") );
		s_extra_time = 0;
		return true;
	}
#ifdef __DNAS_DEBUG__	
	scePrintf( "Available Memory: %d\n", Mem::Manager::sHandle().Available());
#endif

	// Worst case, if we're out of memory because of leaks or fragmentation, just act as if
	// DNAS authentication took place and verified the authenticity of the user
	if( Mem::Manager::sHandle().Available() < (int) ( 1024 * 1024 * 1.7 ))
	{
#ifdef __DNAS_DEBUG__
		scePrintf( "Could not allocate overlay buffer. Available mem: %d. Skipping test\n", Mem::Manager::sHandle().Available());
#endif
		Dbg_MsgAssert( 0, ( "Could not allocate enough safe memory for DNAS. Tell Steve\n" ));
		Script::RunScript( CRCD(0xbd54c02b,"connected_to_internet") );
		s_extra_time = 0;
		return true;
	}

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().BottomUpHeap());
	memory = new char[(int) ( 1024 * 1024 * 1.7 )];
	Mem::Manager::sHandle().PopContext();

#ifdef __DNAS_DEBUG__
	scePrintf( "Memory is at : 0x%x\n", memory );
	scePrintf( "DNAS Start: 0x%x  DNAS Size: %d\n", (int) _dnas_start, (int) _dnas_size );
#endif
	
	if(	((uint32) _dnas_start + (uint32) _dnas_size ) > (uint32) ( memory + (int) ( 1024 * 1024 * 1.7 )) ||
		((uint32) _dnas_start < (uint32) memory ))
	{
		// Worst case, if we're out of memory because of leaks or fragmentation, just act as if
		// DNAS authentication took place and verified the authenticity of the user

		delete [] memory;
#ifdef __DNAS_DEBUG__
		scePrintf( "DNAS didn't fit in the memory that was allocated\n" );
#endif
		Dbg_MsgAssert( 0, ( "Could not allocate enough safe memory for DNAS. Tell Steve\n" ));
		Script::RunScript( CRCD(0xbd54c02b,"connected_to_internet") );
		s_extra_time = 0;
		return true;
	}
		 
	// Clear out the destination memory
	memset( _dnas_start, 0, (int) _dnas_size );

	p_file = NULL;
	if( Config::gLanguage == Config::LANGUAGE_ENGLISH )
	{
		if( Config::gDisplayType == Config::DISPLAY_TYPE_NTSC )
		{
			p_file = File::Open( "dnas/dnas.bin", "rb" );
		}
		else
		{
			p_file = File::Open( "dnas/dnas_e.bin", "rb" );
		}
	}
	else
	{
		p_file = File::Open( "dnas/dnas_e.bin", "rb" );
	}

#ifdef __DNAS_DEBUG__
	if( p_file == NULL )
	{
		scePrintf( "Failed to open dnas/dnas(_e).bin" );
	}
#endif

	File::Read( _dnas_start, 1, (int) _dnas_size, p_file );
	File::Close( p_file );

#ifndef __PURE_HEAP__
	Dbg_MsgAssert( !Config::CD(), ( "DNAS will not work with memory debugging enabled." ));
#endif

#ifdef __USE_DNAS_DLL__
	char* rel_mem, *buff;
	void *p_file = File::Open( "dnas/dnas.rel", "rb" );
	File::Seek( p_file, 0, SEEK_END );
	int32 size = File::Tell( p_file );
	Dbg_Printf( "***************** FILE SIZE IS %d\n", size );
	File::Seek( p_file, 0, SEEK_SET );

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().TopDownHeap());
	rel_mem = new char[size + 127];
	Mem::Manager::sHandle().PopContext();

	buff = (char*) (((int) rel_mem + 127 ) & ~127 ); // Align to 128-byte boundary

	File::Read( buff, sizeof( char ), size, p_file );
	File::Close( p_file );

	if( snDllLoaded( buff, 0 ))
	{
		Dbg_MsgAssert( 0, ( "Failed to load dnas DLL\n" ));
		return false;
	}
#endif

	if( !Config::CD())
	{
		sceCdInit(SCECdINIT);
		sceCdMmode(SCECdDVD);
		sceCdDiskReady( 0 );
	}

	memset( &status, 0, sizeof( sceDNAS2Status_t )); 
	result = netstart_dnas( 0, NULL, 0, vAUTH_TIMEOUT + s_extra_time, status );
	Dbg_Printf( "dnas result was : %d\n", result );
	//install_dnas( 0, NULL, 0, 0 );

	delete [] memory;

	if( result != 0 )
	{
		Script::CStruct* pParams;
		uint32 desc;
		uint32 footer;
		const char* err_desc;
		const char* err_footer;

		s_extra_time += 10;

		pParams = new Script::CStruct;
		if( status.code == 0 )
		{
			pParams->AddInteger( CRCD(0x88eacf67,"code"), 0 );
			pParams->AddInteger( CRCD(0xaf7c50a6,"sub_code"), 0 );
			desc = CRCD(0xc7c67eb8,"net_auth_error_generic");
			if( Config::PAL())
			{
				footer = CRCD(0xc13153cc,"net_auth_footer_customer_pal");
			}
			else
			{
				
				footer = CRCD(0x16ff6a40,"net_auth_footer_contact");
			}
		}
		else
		{
			Dbg_Printf( " Code: %d  SubCode: %d\n", status.code, status.sub_code );
			pParams->AddInteger( CRCD(0x88eacf67,"code"), status.code );
			pParams->AddInteger( CRCD(0xaf7c50a6,"sub_code"), status.sub_code );
			desc = s_get_error_desc( status.sub_code );
			if( Config::PAL())
			{
				footer = s_get_error_footer_pal( status.sub_code );
			}
			else
			{
				footer = s_get_error_footer( status.sub_code );
			}
		}
		
		err_desc = Script::GetString( desc, Script::ASSERT );
		err_footer = Script::GetString( footer, Script::ASSERT );
		Dbg_Printf( "ERROR DESC: %s\n", err_desc );
		Dbg_Printf( "ERROR FOOTER: %s\n", err_footer );
		pParams->AddString( CRCD(0xf44a53ab,"desc"), err_desc );
		pParams->AddString( CRCD(0x1dcefaac,"footer"), err_footer );
		Script::RunScript( CRCD(0xc3bb2b5a,"display_auth_error"), pParams );

		delete pParams;
		return false;
	}

	Dbg_Printf( "Success: Showing disclaimer\n" );
	Script::RunScript( CRCD(0xbd54c02b,"connected_to_internet") );
	s_extra_time = 0;
	return true;
}

bool GameNet::Manager::ScriptWriteDNASBinary( Script::CStruct* pParams, Script::CScript* pScript )
{
	void *p_file;
	static bool written = false;

	if( Config::CD())
	{
		written = true;
	}
	
	if( !written ) 
	{
		if( Config::gLanguage == Config::LANGUAGE_ENGLISH )
		{
			if( Config::gDisplayType == Config::DISPLAY_TYPE_NTSC )
			{
				p_file = File::Open( "dnas/dnas.bin", "wb" );
			}
			else
			{
				p_file = File::Open( "dnas/dnas_e.bin", "wb" );
			}
		}
		else
		{
			p_file = File::Open( "dnas/dnas_e.bin", "wb" );
		}
		File::Write( _dnas_start, 1, (int) _dnas_size, p_file );
		File::Flush( p_file );
		File::Close( p_file );

		written = true;
	}

	return true;
}


