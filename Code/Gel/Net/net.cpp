/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			Net (OBJ) 												**
**																			**
**	File name:		net.cpp													**
**																			**
**	Created:		01/29/01	-	spg										**
**																			**
**	Description:	Network Manager code									**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <gel/net/net.h>

#include <core/defines.h>

#include <sys/timer.h>
#include <sys/config/config.h>
#include <sys/sioman.h>
#include <sys/file/filesys.h>

#include <stdlib.h>
#include <string.h>

#include <gel/net/server/netserv.h>
#include <gel/net/client/netclnt.h>

#ifndef __PLAT_WN32__
#include <gel/music/music.h>
#endif

#ifdef __PLAT_XBOX__
//#include <xonline.h>
#include <winsockx.h>
#endif

#include <gel/mainloop.h>

#ifdef __PLAT_NGPS__

#include <sifdev.h>
#include <libcdvd.h>

#endif
/*****************************************************************************
**								  Externals									**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

//#define USE_DECI2

#define DEFAULT_SNPS2_SUB_MSK	"255.255.255.0"
#define DEFAULT_SNPS2_GATEWAY	"192.168.0.1"

const char  spduartArgs[]   = "-nogci\0dial=cdrom0:\\IOP\\DIAL_SPD.CNF;1";

/*****************************************************************************
**								DBG Defines									**
*****************************************************************************/



namespace Net
{



/*****************************************************************************
**								Private Types								**
*****************************************************************************/

/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

DefineSingletonClass( Manager, "Network Manager" )

#ifdef __PLAT_NGPS__
/* Set up the following list with one or more suitable DNS servers   */

/*static const sn_char* dns_servers[] =
{   
	    "205.147.0.100",
		"205.147.0.102",
		""                // List is terminated by null string
};*/
static sn_char custom_built_script[SN_MAX_SCRIPT_LINES][SN_MAX_SCRIPT_LEN+1];
static const char *s_custom_isp_script[] =
    {
        "input 30 ogin:",      /* Line 0 */
        "output ",             /* Line 1 */
        "input 10 word:",      /* Line 2 */
        "output ",             /* Line 3 */
        "input 10 ing PPP",    /* Line 4 */
        ""                     /* script is terminated with null string     */
    };

#define vCUSTOM_USERNAME_LINE  1
#define vCUSTOM_PASSWORD_LINE  3

static bool s_cancel_dialup_conn;
static int  s_conn_semaphore;
#endif // __PLAT_NGPS__

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/


/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

#ifdef __PLAT_NGPS__

#define vPOWEROFF_STACK_SIZE		(2 * 1024)

static u_char s_poweroff_stack[vPOWEROFF_STACK_SIZE] __attribute__ ((aligned(16)));

/******************************************************************/
/* The thread which waits for			                                  					  */
/*                                                                */
/******************************************************************/

static void s_power_off_thread(void *arg) 
{
	int sid = (int)arg;
	int stat;
	while( 1 ) 
	{
		WaitSema(sid);
		// dev9 power off, need to power off PS2
		//while( sceDevctl("dev9x:", DDIOC_OFF, NULL, 0, NULL, 0 ) < 0 );
		while( sceDevctl("dev9x:", DDIOC_OFF, NULL, 0, NULL, 0 ) < 0 );
		// PS2 power off
		while( !sceCdPowerOff( &stat ) || stat );
    }
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

static void s_power_off_handler(void *arg) 
{
	int sid = (int)arg;
    iSignalSema(sid);
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

static void s_prepare_power_off(void) 
{
    struct ThreadParam tparam;
    struct SemaParam   sparam;
    int tid, sid;

	sparam.initCount = 0;
    sparam.maxCount  = 1;
    sparam.option    = 0;
    sid = CreateSema(&sparam);

    tparam.stackSize = vPOWEROFF_STACK_SIZE;
    tparam.gpReg = &_gp;
    tparam.entry = s_power_off_thread;
    tparam.stack = (void *) s_poweroff_stack;
    tparam.initPriority = 1;
    tid = CreateThread( &tparam );
    StartThread( tid, (void *) sid );

    sceCdPOffCallback( s_power_off_handler, (void *) sid );
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

static sn_int32 set_host_name( sn_char* host_name )
{
    sndev_set_dhost_type optval;
    sn_int32 host_size, r;

	

	Dbg_Assert( host_name );

    host_size = strlen(host_name) + 1;

    // Check host_name isn't too big
    if (host_size > (sn_int32) sizeof(optval.host))
    {
        Dbg_Printf("EE:set_host_name():error host_name too big\n");
        return -1;
    }

    // I'm going to fill optval with zeros to start with, not strictly
    // necessary but this is only an example.
    memset(&optval,0,sizeof(optval));

    // Fill in the optval structure
    optval.flags = 0;        // Steve, you could set one of these two
                             // values SN_DFLAG_EXCL_DIS or
                             // SN_DFLAG_EXCL_REQ in the flags to
                             // *exclude* either the discovery or
                             // request message from the operation with
                             // flags set to zero the option will be
                             // applied to both messages

	if( host_name[0] == '\0' )
	{
		Dbg_Printf( "Clearing Host Name Option\n" );
		optval.clear_option = 1; // Steve, you would set this to 1 if you
								 // wanted to remove the host name
	}
	else
	{
		Dbg_Printf( "Setting Host Name Option to %s\n", host_name );
		optval.clear_option = 0;
	}
    

    optval.include_null = 0; // Steve, if you wanted the null terminator
                             // included in the host name that's sent in
                             // the DHCP msg you would set this to 1

    optval.reserved = 0;    // Must be 0

    memcpy(optval.host, host_name, host_size);

    // Send the option to the IOP

    r = sndev_set_options(0, SN_DEV_SET_DHOST, &optval, sizeof(optval));

    if (r != 0)
    {
        Dbg_Printf("EE:sndev_set_options():error %d\n",r);
        return -1;
    }

    return 0;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

bool	Manager::stop_stack( void )
{
	

	int result, stack_state;

	Dbg_Printf( "EE:Stopping the TCP/IP stack\n" );
	
	result = sn_stack_state(SN_STACK_STATE_STOP, &stack_state);

	if( result != 0 )
	{
		Dbg_Printf( "EE:sn_stack_sate() failed %d\n", result );
		SetError( vRES_ERROR_GENERAL );
		return false;
	}

	Dbg_Printf( "EE:Stack Stopped\n" );

	return true;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

bool	Manager::start_stack( void )
{
	sn_int32	result, stack_state;
	Tmr::Time 	start_time;

	

	// Start the stack
    Dbg_Printf("EE:Starting the TCP/IP stack\n");

    result = sn_stack_state( SN_STACK_STATE_START, &stack_state );
    
    if( result != 0 )
    {
        Dbg_Printf( "EE:sn_stack_sate() failed %d\n", result );
		SetError( vRES_ERROR_GENERAL );
        return false;
    }

	Dbg_Printf( "EE:Stack Started\n" );
	if(( GetConnectionType() == vCONN_TYPE_MODEM ) ||
	   ( GetConnectionType() == vCONN_TYPE_PPPOE ))
	{
		int modem_state, prev_modem_state;
		Tmr::Time start_time;

        modem_state      = -1; // Invalid
		prev_modem_state = -2; // Ivalid and != modem_state

		start_time = Tmr::GetTime();
		Dbg_Printf( "EE:Calling snmdm_get_state() - until modem ready\n" );
		while( modem_state != SN_MODEM_READY )
		{
			result = snmdm_get_state( &modem_state );
	
			if (result != 0)
			{
				Dbg_Printf("EE:snmdm_get_state() failed %d\n", result);
				SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
				return false;
			}
	
			if (modem_state == prev_modem_state)
			{
				sn_delay(10);
			}
			else
			{
				prev_modem_state = modem_state;
				Dbg_Printf("  Modem state = %d %s\n", modem_state, sntc_str_modem_state( modem_state ));
			}

			// After 5 seconds, time out and say there was no dialtone
			if(( Tmr::GetTime() - start_time ) > Tmr::Seconds( 10 ))
			{
				SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
				
				return false;
			}
		}

		Dbg_Printf( "EE:Ready\n" );
	}


    // Wait for the stack to come up
    Dbg_Printf( "EE:Waiting for socket API to be ready\n" );
	start_time = Tmr::GetTime();
    while( sn_socket_api_ready() == SN_FALSE )
	{
		// Delay to avoid hogging the processor
		sn_delay( 500 );
		if(( Tmr::GetTime() - start_time ) > Tmr::Seconds( 10 ))
		{
			Dbg_Printf( "EE:Timed out waiting for socket API to be ready\n" );
			return false;
		}
    }

	if( ShouldUseDHCP())
	{
		struct hostent* hentp       = NULL;
		sn_bool         got_ip_addr = SN_FALSE;
		struct in_addr  ip_addr;
	
		Dbg_Printf( "EE:Waiting for DHCP server to supply IP addr etc\n" );
		start_time = Tmr::GetTime();
		do
		{
			// A way of getting the local IP address
			hentp = gethostbyname(LOCAL_NAME);
	
			if(( hentp != NULL ) && ( hentp->h_addr_list[0] != NULL ))
			{
				// Read the IP address from the hostent struct
				memcpy( &ip_addr,hentp->h_addr_list[0], sizeof( ip_addr ));
	
				if( ip_addr.s_addr != 0 )
				{
					got_ip_addr = SN_TRUE;
					Dbg_Printf( "DHCP allocated IP addr %s\n", inet_ntoa( ip_addr ));
				}
			}
	
			// Delay to avoid hogging the processor
			if( got_ip_addr == SN_FALSE )
			{
				sn_delay(500);
			}
			if(( Tmr::GetTime() - start_time ) > Tmr::Seconds( 10 ))
			{
				Dbg_Printf( "EE:Timed out waiting for DHCP response\n" );
				SetError( vRES_ERROR_DHCP );
				return false;
			}
		} while( got_ip_addr == SN_FALSE );

		strcpy( m_local_ip, inet_ntoa( ip_addr ));
    }

	return true;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

bool	Manager::setup_ethernet_params( void )
{
	sndev_set_ether_ip_type params;
	int result;

    if( ShouldUseDHCP())
    {
		Dbg_Printf( "\n\n\n*********************** USING DHCP ******************* \n\n\n" );
        memset( &params,0, sizeof( params ));
    }
    else
    {   
        inet_aton( GetLocalIP(), (struct in_addr*) &params.ip_addr );
		Dbg_Printf( "======================== Ip is %d : %d\n", params.ip_addr, htonl( params.ip_addr ));
        inet_aton( GetSubnetMask(), (struct in_addr*) &params.sub_mask );
        inet_aton( GetGateway(), (struct in_addr*) &params.gateway );
	}

    result = sndev_set_options( 0, SN_DEV_SET_ETHER_IP, &params, sizeof(params));

    if( result != 0 )
    {
        Dbg_Printf( "EE:Error sndev_set_options() returned %d\n", result );
		SetError( vRES_ERROR_GENERAL );
        return false;
    }

	if( ShouldUseDHCP())
	{
		result = set_host_name( m_host_name );
		if( result != 0 )
		{   
			SetError( vRES_ERROR_GENERAL );
			return false;
		}
	}

	return true;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

bool	Manager::initialize_device( void )
{
	sn_int32            result;
    sn_int32            device_type;
    sn_int16            idVendor;
    sn_int16            idProduct;
    sn_bool             first_time;
	Tmr::Time 			start_time;
	Tmr::Time			timeout;

    // Initialise the socket API, if fails print error and return
    Dbg_Printf( "EE:Initialising socket API\n" );

    result = sockAPIinit( 6 );
    if(	( result != 0 ) &&
		( result != SN_EALRDYINIT ))
    {
        Dbg_Printf( "EE:sockAPIinit() failed %d\n", result );
		SetError( vRES_ERROR_GENERAL );
        return false;
    }
    
    // If we've already init'd the sockets API, don't re-register this thread
	// just let it run through the rest of setup
	if( result != SN_EALRDYINIT )
	{
		// Register this thread with the socket API
		result = sockAPIregthr();
	
		if (result != 0)
		{
			Dbg_Printf( "EE:sockAPIregthr() failed %d\n", result );
			SetError( vRES_ERROR_GENERAL );
			return false;
		}
	}

    // Now wait for DECI2 'device' to be 'attached'
    device_type = SN_DEV_TYPE_NONE;
    first_time  = SN_TRUE;
	start_time = Tmr::GetTime();
	// It takes longer to init the sony modem
	if( GetDeviceType() == vDEV_TYPE_SONY_MODEM )
	{
		timeout = Tmr::Seconds( 15 );
	}
	else
	{
		timeout = Tmr::Seconds( 10 );
	}
	
    while( device_type == SN_DEV_TYPE_NONE )
    {
        result = sndev_get_attached( 0, &device_type, &idVendor, &idProduct );

        if( result != 0 )
        {
            Dbg_Printf( "EE:sndev_get_attached() failed %d\n", result );
			SetError( vRES_ERROR_GENERAL );
            return false;
        }

        if( device_type == SN_DEV_TYPE_NONE )
        {
            if( first_time == SN_TRUE )
            {
                first_time = SN_FALSE;
                Dbg_Printf( "EE:Waiting for network device to be attached ...\n" );
            }
            sn_delay( 10 );
        }

		if(( Tmr::GetTime() - start_time ) > timeout )
		{
			Dbg_Printf( "EE:Timed out waiting for network device to be attached\n" );
			break;
		}
    }

	switch( device_type )
	{
		case SN_DEV_TYPE_DECI2:
			Dbg_Printf( "Using DECI-2 ethernet emulation\n" );
			break;
		case SN_DEV_TYPE_USB_MODEM:
			Dbg_Printf("EE:USB-Modem Attached (idVendor=0x%04X idProduct=0x%04X)\n",
					((int)idVendor)  & 0xFFFF,
					((int)idProduct) & 0xFFFF);
			break;
		case SN_DEV_TYPE_USB_ETHER:
			Dbg_Printf("EE:USB-Ethernet Attached (idVendor=0x%04X idProduct=0x%04X)\n",
					((int)idVendor)  & 0xFFFF,
					((int)idProduct) & 0xFFFF);
			break;
		default:
		{
			if( idVendor == 0 )
			{
				Dbg_Printf( "Unknown Device %d\n", device_type );
				SetError( vRES_ERROR_UNKNOWN_DEVICE );
			}
			else
			{
				Dbg_Assert( idVendor == 1 );
				SetError( vRES_ERROR_DEVICE_NOT_HOT );
			}
			
			return false;
		}
	}
    
	return true;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

bool	Manager::sn_stack_setup( void )
{
	// Set up the list of DNS servers
	char* dns_servers[3];
	int result, i;
	sn_int32	stack_state;
		
	

	sn_stack_state( SN_STACK_STATE_READ, &stack_state );
	if( stack_state == SN_STACK_STATE_START )
	{
		if( stop_stack() == false )
		{
			return false;
		}
	}
		
	if( GetConnectionType() == vCONN_TYPE_ETHERNET ) 
	{
		if(( setup_ethernet_params() == false ))
		{
			return false;
		}
	}

	for( i = 0; i < 3; i++ )
	{
		dns_servers[i] = m_dns_servers[i];
		Dbg_Printf( "DNS Server %d is %s\n", i, dns_servers[i] );
	}
	result = sntc_set_dns_server_list((const sn_char**) dns_servers);

	if( result != 0 )
	{
		Dbg_Printf( "EE:sntc_set_dns_server_list() failed %d\n", result );
		SetError( vRES_ERROR_GENERAL );
		
		return false;
	}

	if( GetConnectionType() == vCONN_TYPE_PPPOE )
	{
		sndev_set_pppoe_opt_type pppoe;

		// Enable PPPoE
		pppoe.flags = 1;

		Dbg_Printf( "Enabling PPPoE\n" );
		result = sndev_set_options( 0, SN_DEV_SET_PPPOE_OPT, &pppoe, sizeof(pppoe));

		Dbg_Printf( "EE:sndev_set_options(pppoe) returned %d\n", result );
	}
	if(( GetConnectionType() == vCONN_TYPE_MODEM ) ||
	   ( GetConnectionType() == vCONN_TYPE_PPPOE ))
	{
		// Now, we have a valid modem. Try to initialize
		//result = snmdm_set_mdm_init( "AT&F S0=0" );
		result = snmdm_set_mdm_init( "AT&F S0=0 W2" );
	
		if( result != 0 )
		{
			Dbg_Printf( "EE:snmdm_set_mdm_init() failed %d\n", result );
			SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
			
			return false;
		}

		Dbg_Printf( "EE:snmdm_set_mdm_init() worked ok\n" );
	}

    if( start_stack() == false )
	{
		return false;
	}

	m_options_changed = false;
	return true;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

static sn_int32 custom_construct_script( sn_int32 isp_type, sn_char* user_name, 
										 sn_char* password)
{
    sn_int32  line_index;
    sn_bool   copy_done;

    copy_done  = SN_FALSE;
    line_index = 0;

    // Copy the preset script to custom_built_script
    while( copy_done == SN_FALSE )
    {
        // Copy this line of the script
        strcpy( custom_built_script[line_index], s_custom_isp_script[line_index] );

        // If this line was a null string then finished copy
        if( s_custom_isp_script[line_index][0] == 0 )
		{
             copy_done = SN_TRUE;
		}
        else
		{
			line_index++;
		}
    }

    sn_strcat( custom_built_script[vCUSTOM_USERNAME_LINE], user_name );
    sn_strcat( custom_built_script[vCUSTOM_USERNAME_LINE], "\\r" );

    // Concatenate password\r on to line index CUSTOM_PASSWORD_LINE
    sn_strcat( custom_built_script[vCUSTOM_PASSWORD_LINE], password );
    sn_strcat( custom_built_script[vCUSTOM_PASSWORD_LINE], "\\r" );

    return 0; // Success
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

sn_int32 custom_connect_modem(	sn_char*               phone_no,
								sn_int32               isp_type,
								sn_char*               user_name,
								sn_char*               password,
								sn_int32               timeout_secs,
								sntc_mdmstate_callback callback )
{
    sn_int32           result;
    sn_int32           modem_state;
    sn_bool            done_script;
    sn_int32           script_index;
    sn_int32           prev_modem_state;
    sn_bool            connect_started;
    sn_int32           timeout_ms;
    sn_int32           connect_err;
    
    result = snmdm_set_phone_no( phone_no );

    // Check that the above function call worked ok
    if( result != 0 )
    {
		Dbg_Printf( "Failed to set phone number: %d\n", result );
        return SNTC_ERR_MDMAPI;
    }

    // Construct the log in script file (in custom_built_script)
    result = custom_construct_script( isp_type, user_name, password );
    if( result != 0 )
    {
        Dbg_Printf( "Failed to build login script: %d\n", result );
        return SNTC_ERR_BSCRIPT;
    }

    // Store the log in script file via the modem API
    // Send a null string first, this resets the script write ptr to 0,
    // it should already be at 0, but just being defensive.
    result = snmdm_set_script("");
    if( result != 0 )
    {
		Dbg_Printf( "snmdm_set_script() failed: %d\n", result );
        return SNTC_ERR_MDMAPI;
    }

    // Now send the script file to the modem API
    script_index = 0;
    done_script  = SN_FALSE;
    while( done_script == SN_FALSE )
    {
        result = snmdm_set_script( custom_built_script[ script_index ] );
        if( result != 0 )
        {
			Dbg_Printf( "snmdm_set_script() failed: %d\n", result );
            return SNTC_ERR_MDMAPI;
        }

        // Check for null line, which is last line
        if( custom_built_script[script_index][0] == 0 )
		{
			 done_script = SN_TRUE;
		}
		else
		{
			script_index++;
		}
    }

    // Everything is ready, so ask the modem to connect
	result = snmdm_connect();
    if( result != 0 )
    {
		Dbg_Printf( "snmdm_connect() failed: %d\n", result );
        return SNTC_ERR_MDMAPI;
    }

    // Now wait for the modem to become connected
    modem_state      = -1; /* Invalid */
    prev_modem_state = -2; /* Ivalid and != modem_state */
    connect_started  = SN_FALSE;
    timeout_ms       = timeout_secs * 1000;

    while(( modem_state != SN_MODEM_PPP_UP ) &&
		  ( s_cancel_dialup_conn == false ))
    {

        // Get the current state of the modem link
        result = snmdm_get_state(&modem_state);
        if( result != 0 )
        {
			Dbg_Printf( "snmdm_get_state() failed: %d\n", result );
            return SNTC_ERR_MDMAPI;
        }

        // Monitor for modem connection process starting, then if it
        // goes back to ready we know it's failed to connect.
        if( connect_started == SN_FALSE )
        {
            // Any of the following means modem connection started
            if(  (modem_state == SN_MODEM_DIALING)
               ||(modem_state == SN_MODEM_LOGIN)
               ||(modem_state == SN_MODEM_CONNECTED)
               ||(modem_state == SN_MODEM_PPP_UP))
            {
                connect_started = SN_TRUE;
            }
        }
        else
        {
            // Having started the connection, then unless it's in
            // one of the following states it's failed to connect
            if(  (modem_state != SN_MODEM_DIALING)
               &&(modem_state != SN_MODEM_LOGIN)
               &&(modem_state != SN_MODEM_CONNECTED)
               &&(modem_state != SN_MODEM_PPP_UP))
            {
                // Read the reason why the connect failed.
                result = snmdm_get_connect_err( &connect_err );
                if( result != 0 )
                {
					Dbg_Printf( "snmdm_get_connect_err() failed: %d\n", result );
                    return SNTC_ERR_MDMAPI;
                }

                switch( connect_err )
                {
                    case SN_CONERR_BUSY:
                       Dbg_Printf( "Busy\n" );
                       return SNTC_ERR_BUSY;
                    break;

                    case SN_CONERR_NOCARRIER:
                       Dbg_Printf( "No carrier\n" );
                       return SNTC_ERR_NOCARRIER;
                    break;

					case SN_CONERR_NOANSWER:
						Dbg_Printf( "No answer\n" );
						return SNTC_ERR_NOANSWER;
                    break;

					case SN_CONERR_NODIALTONE:
						Dbg_Printf( "No dialtone\n" );
						return SNTC_ERR_NODIALTONE;
                    break;

                    default:
                        Dbg_Printf( "connect started then modem state=sntc_str_modem_state(modem_state)\n" );
                        return SNTC_ERR_CONNECT;
                }
            }
        }

        // Now check whether the modem state has changed since the
        // previous time round this loop, and if so call the user
        // callback function (unless it's NULL), otherwise check
        // for time-out / do a delay
        if( modem_state != prev_modem_state )
        {
            prev_modem_state = modem_state;
            if( callback != NULL )
			{
				(*callback)(modem_state);
			}
        }
        else
        {
            if( timeout_ms <= 0 )
            {
				Dbg_Printf( "Connect timed out in modem state %s\n", sntc_str_modem_state(modem_state) );
                return SNTC_ERR_TIMEOUT;
            }

            sn_delay( 10 );
            timeout_ms -= 10;
        }
    }

    // If we get to here, the modem has successfully connected
    Dbg_Printf( "Connected!\n" );
    
	return 0;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

void	Manager::threaded_modem_conn( void *data )
{
	
	Manager* man = (Manager *) data;
	int result;
    
	Dbg_Printf( "Registering modem thread %d with stack\n", GetThreadId());
	
	result = sockAPIregthr();
	Dbg_Assert( result == 0 );

	WaitSema( s_conn_semaphore );

	Dbg_Printf("EE:Calling sntc_connect_modem()\n");

	// Set the script write pointer back to zero
    result = snmdm_set_script("");

	// Set up Authentication parameters.
	sndev_set_chap_type ChapOptions = {0};
	
    ChapOptions.accept_chap = man->ShouldUseDialupAuthentication();
	Dbg_Printf( "Setting chap to %d\n", ChapOptions.accept_chap );
	ChapOptions.require_chap = 0;
	strcpy(ChapOptions.locl_name, man->m_isp_user_name );
	strcpy(ChapOptions.locl_secr, man->m_isp_password );
	strcpy(ChapOptions.chal_name,"*"); // Accept any challenge name
	
	Dbg_Printf( "Chap username: %s password: %s\n", man->m_isp_user_name, man->m_isp_password );
	result = sndev_set_options( 0, SN_DEV_SET_CHAP, &ChapOptions, sizeof(ChapOptions));
	Dbg_Printf( "EE:sndev_set_options(SN_DEV_SET_CHAP) returned %d\n", result );

	sndev_set_pap_type PapOptions = {0};

	PapOptions.accept_pap = man->ShouldUseDialupAuthentication();
	Dbg_Printf( "Setting pap to %d\n", PapOptions.accept_pap );
	PapOptions.require_pap = 0;
	strcpy(PapOptions.locl_name, man->m_isp_user_name );
	strcpy(PapOptions.locl_pass, man->m_isp_password );
	Dbg_Printf( "Pap username: %s password: %s\n", man->m_isp_user_name, man->m_isp_password );

	result = sndev_set_options( 0, SN_DEV_SET_PAP, &PapOptions, sizeof(PapOptions));
	Dbg_Printf( "EE:sndev_set_options(SN_DEV_SET_PAP) returned %d\n", result );

	man->m_modem_err = 0;
	if( man->GetConnectionType() == vCONN_TYPE_MODEM )
	{   
		Dbg_Printf( "Dialing %s user: %s pass: %s\n", man->m_isp_phone_no, man->m_isp_user_name, man->m_isp_password );
		result = custom_connect_modem
				 ( man->m_isp_phone_no,			// phone_no
				   SNTC_ISP_GENERIC,		// isp_type
				   man->m_isp_user_name,			// user_name
				   man->m_isp_password,			// password
				   vMODEM_CONNECT_TIMEOUT,	// timeout_secs
				   man->conn_modem_state_callback );// callback
	}
	else if( man->GetConnectionType() == vCONN_TYPE_PPPOE )
	{
		result = custom_connect_modem
				 ( "",			// phone_no
				   0,		// isp_type
				   "",			// user_name
				   "",			// password
				   vMODEM_CONNECT_TIMEOUT,	// timeout_secs
				   man->conn_modem_state_callback );// callback
	}
	// Check whether connection succeeded
	if( result == 0 )
	{
        sn_int32 result, statval, statlen;

		statval = 1234; // so can see it's modified
		statlen = sizeof(statval);

        result = sndev_get_status(0, SN_DEV_STAT_BAUD, &statval, &statlen);
        man->m_modem_baud_rate = statval;
		
		man->m_online = true;
	}
	else
	{   
		sntc_disconnect_modem(	vMODEM_DISCONNECT_TIMEOUT,	// timeout_secs
								NULL,		// callback
								NULL );				// error_message
		man->SetModemState( vMODEM_STATE_ERROR );
		man->m_modem_err = result;
		Dbg_Printf( "EE:sntc_connect_modem() failed: %d\n", result );
		man->m_online = false;
	}   

	Dbg_Printf( "DeRegistering modem thread %d with stack\n", GetThreadId());
	sockAPIderegthr();
	SignalSema( s_conn_semaphore );
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

void	Manager::threaded_modem_disconn( void *data )
{
	
	Manager* man = (Manager *) data;
	sn_char* err_msg_ptr;
	bool modem_disconnected;
	int result;
    
	Dbg_Printf( "Registering modem thread %d with stack\n", GetThreadId());
	
	result = sockAPIregthr();
	Dbg_Assert( result == 0 );
		
	modem_disconnected = false;
	if( man->m_online )
	{
		Dbg_Printf( "EE:Calling sntc_disconnect_modem()\n" );
		result = sntc_disconnect_modem(	vMODEM_DISCONNECT_TIMEOUT,	// timeout_secs
										disconn_modem_state_callback,		// callback
										&err_msg_ptr );				// error_message
	
		// Check whether disconnection succeeded
		if (result == 0)
		{
			Dbg_Printf("EE:sntc_disconnect_modem() worked:%s\n",err_msg_ptr);
			modem_disconnected = true;
		}
		else
		{
			Dbg_Printf("EE:sntc_disconnect_modem() failed:%s\n",err_msg_ptr);
			modem_disconnected = false;
		}
	}

	// If the modem didn't connect and disconnect ok, then reset it
	if( !modem_disconnected )
	{
		bool modem_reset;

		do
		{
			Dbg_Printf( "EE:Calling sntc_reset_modem()\n" );

			result = sntc_reset_modem
						(	vMODEM_RESET_TIMEOUT,	// timeout_secs
							disconn_modem_state_callback,	// callback
							&err_msg_ptr );     	// error_message

			// Check whether reset succeeded
			if( result == 0 )
			{
				Dbg_Printf( "EE:sntc_reset_modem() worked:%s\n", err_msg_ptr );
				modem_reset = true;
			}
			else
			{
				Dbg_Printf( "EE:sntc_reset_modem() failed:%s\n", err_msg_ptr );
				modem_reset = false;
				// If their modem is no longer plugged in, just consider it "hung up"
				if( result == SNTC_ERR_NOMODEM )
				{
					break;
				}
				sn_delay( 1000 ); // avoid excessive printf if unplugged
			}
		} while( !modem_reset );
	}
	
	man->SetModemState( vMODEM_STATE_DISCONNECTED );
	man->m_online = false;

	Dbg_Printf( "DeRegistering modem thread %d with stack\n", GetThreadId());
	sockAPIderegthr();
}

#endif	// __PLAT_NGPS__

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/* Add logic tasks to the current task list                       */
/*                                                                */
/******************************************************************/

void	Manager::AddLogicTasks( App* app )
{
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
    
	mlp_manager->AddLogicTask( app->GetReceiveDataTask());
	mlp_manager->AddLogicTask( app->GetSendDataTask());
	mlp_manager->AddLogicTask( app->GetProcessDataTask());
	mlp_manager->AddLogicTask( app->GetNetworkMetricsTask());
}

/******************************************************************/
/* Add logic tasks to the push logic task list                    */
/*                                                                */
/******************************************************************/

void	Manager::AddLogicPushTasks( App* app )
{
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();

	app->GetSendDataTask().Remove();
	app->GetReceiveDataTask().Remove();
	app->GetProcessDataTask().Remove();
	app->GetNetworkMetricsTask().Remove();

	mlp_manager->AddLogicPushTask( app->GetReceiveDataTask());
	mlp_manager->AddLogicPushTask( app->GetSendDataTask());
	mlp_manager->AddLogicPushTask( app->GetProcessDataTask());
	mlp_manager->AddLogicPushTask( app->GetNetworkMetricsTask());
}

/******************************************************************/
/* Removes network logic tasks                    				  */
/*                                                                */
/******************************************************************/

void	Manager::RemoveNetworkTasks( App* app )
{
	app->GetSendDataTask().Remove();
	app->GetReceiveDataTask().Remove();
	app->GetProcessDataTask().Remove();
	app->GetNetworkMetricsTask().Remove();
}

/******************************************************************/
/* Creates a new Server at the given address and port			  */
/*                                                                */
/******************************************************************/

Server*	Manager::CreateNewAppServer( int id, char* appName, int max_clients, 
									  unsigned short port, int address, int flags )
{
	Server *new_app;
		
	

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	// If we're using DHCP, we should have gotten our real IP address by now
	// from the DHCP server. Use that address from now on
	if( ShouldUseDHCP())
	{
		address = inet_addr( m_local_ip );		
	}

	new_app = new Server( flags );
	new_app->m_net_man = this;
	new_app->init();
    
	new_app->m_id = id;	
	strncpy( new_app->m_name, appName, MAX_LEN_APP_NAME );
	new_app->m_max_connections = max_clients;
    
	m_net_servers.AddToTail( &new_app->m_node );
	if( new_app->IsLocal())
	{                  
		new_app->m_connected = true;
	}
	else
	{
		new_app->bind_app_socket( address, port );
		if( flags & App::mBROADCAST )
		{
			new_app->m_connected = true;
		}
	}

#ifdef USE_ALIASES
	if( flags & App::mALIAS_SUPPORT )
	{
		new_app->AllocateAliasTables();
		new_app->ClearAliasTables();
	}
#endif
    
	AddLogicTasks( new_app );

#ifndef __PLAT_NGC__
    Dbg_Printf( "Created new: %s server %p, Max Clients: %d, IP: %s, Port: %d", appName, new_app, max_clients,
		   					inet_ntoa( *(struct in_addr*) &address ), port );
#endif		// __PLAT_NGC__

	m_num_apps++;
	
	Mem::Manager::sHandle().PopContext();
	return new_app;
}

/******************************************************************/
/* Creates a new client socket									  */
/*                                                                */
/******************************************************************/

Client	*Manager::CreateNewAppClient( 	int id, char* appName, unsigned short port, int address,
										int flags )
{
	Client *new_app;

	

	Mem::Manager::sHandle().PushContext(Mem::Manager::sHandle().NetworkHeap());

	// If we're using DHCP, we should have gotten our real IP address by now
	// from the DHCP server. Use that address from now on
	if( ShouldUseDHCP())
	{
		address = inet_addr( m_local_ip );
	}

	new_app = new Client( flags );
	strncpy( new_app->m_name, appName, MAX_LEN_APP_NAME );
	new_app->m_net_man = this;
    new_app->init();

	new_app->m_id = id; 
	new_app->m_max_connections = 1;
    
	m_net_clients.AddToTail( &new_app->m_node );
	if( !new_app->IsLocal())
	{
#ifndef __PLAT_XBOX__
		new_app->bind_app_socket( address, port );
#endif
	}
	
#ifdef USE_ALIASES
	new_app->ClearAliasTable();
#endif
    
	AddLogicTasks( new_app );

	Dbg_Printf( "Created new: %s client %p on port %d\n", appName, new_app, port );
	m_num_apps++;

	Mem::Manager::sHandle().PopContext();
	return new_app;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::DestroyApp( App *app )
{
	app->m_node.Remove();
	app->ShutDown();
	delete app;
	m_num_apps--;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

Metrics::Metrics( void )
{
	m_num_packets = 0;
	m_total_bytes = 0;
	m_bytes_per_sec = 0;
	memset( m_num_messages, 0, sizeof( int ) * MAX_MSG_IDS );
	memset( m_size_messages, 0, sizeof( int ) * MAX_MSG_IDS );
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

void	Metrics::CalculateBytesPerSec( int cur_time )
{
	int i, num_bytes;
		
	num_bytes = 0; 
	// Sum up number of bytes transferred over the last second
	for( i = 0; i < vNUM_BUFFERED_PACKETS; i++ )
	{
		if( i >= m_num_packets )
		{
			break;
		}

		if(( cur_time - m_packets[i].GetTime()) < (int) Tmr::Seconds( 1 ))
		{
			num_bytes += m_packets[i].GetNumBytes();
		}
	}

	m_bytes_per_sec = num_bytes;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

int		Metrics::GetBytesPerSec( void )
{
	return m_bytes_per_sec;
}
	
/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

int		Metrics::GetTotalBytes( void )
{
	return m_total_bytes;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

int		Metrics::GetTotalMessageData( int msg_id )
{
	return m_size_messages[ msg_id ];
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

int		Metrics::GetTotalNumMessagesOfId( int msg_id )
{
	return m_num_messages[ msg_id ];
}
	
/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

void	Metrics::AddPacket( int size, int time )
{
	int index;

	index = m_num_packets % vNUM_BUFFERED_PACKETS;

	m_packets[ index ].SetNumBytes( size );
	m_packets[ index ].SetTime( time );
	m_total_bytes += size;
	m_num_packets++;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

void	Metrics::AddMessage( int msg_id, int size )
{
	m_num_messages[ msg_id ]++;
	m_size_messages[ msg_id ] += size;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

int		PacketInfo::GetNumBytes( void )
{
	return m_num_bytes;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

int		PacketInfo::GetTime( void )
{
	return m_time;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/

void	PacketInfo::SetNumBytes( int size )
{
	m_num_bytes = size;
}

/******************************************************************/
/* 			                                  					  */
/*                                                                */
/******************************************************************/
	
void	PacketInfo::SetTime( int time )
{
	m_time = time;
}

/******************************************************************/
/* Iterator                                                       */
/*                                                                */
/******************************************************************/

Server *Manager::FirstServer( Lst::Search< App > *sh )
{
	

	Dbg_Assert( sh );

	return((Server*) sh->FirstItem( m_net_servers ));
}

/******************************************************************/
/* Iterator                                                       */
/*                                                                */
/******************************************************************/

Client *Manager::FirstClient( Lst::Search< App > *sh )
{
	

	Dbg_Assert( sh );

	return((Client*) sh->FirstItem( m_net_clients ));
}

/******************************************************************/
/* Iterator                                                       */
/*                                                                */
/******************************************************************/

App *Manager::NextApp( Lst::Search< App > *sh )
{
	

	Dbg_Assert( sh );

	return( sh->NextItem());
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	Manager::NumApps( void )
{
	return m_num_apps;
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

#ifdef __PLAT_NGC__
static	void*    s_so_alloc( u32 name, s32 size )
{
	return Mem::Malloc( size );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

static	void     s_so_free( u32 name, void* ptr, s32 size )
{
	Mem::Free( ptr );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::initialize_ngc( void )
{
	int result;
    SOConfig config;

	memset( &config, 0, sizeof( SOConfig ));
	
	config.vendor = SO_VENDOR_NINTENDO;
	config.version = SO_VERSION;
	config.alloc = s_so_alloc;
	config.free = s_so_free;
	if( ShouldUseDHCP())
    {
		config.flag = SO_FLAG_DHCP;
    }
    else
    {   
		config.flag = 0;

        inet_aton( GetLocalIP(), (struct in_addr*) &config.addr );
        inet_aton( GetSubnetMask(), (struct in_addr*) &config.netmask );
        inet_aton( GetGateway(), (struct in_addr*) &config.router );
		inet_aton( GetDNSServer( 0 ), (struct in_addr*) &config.dns1 );
		inet_aton( GetDNSServer( 1 ), (struct in_addr*) &config.dns2 );
	}

	result = SOStartup( &config );
	if( result != 0 )
	{
		SOCleanup();
		return false;
	}

	return true;
}

#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#ifdef __PLAT_NGPS__

bool	Manager::load_irx_files( void )
{

//////////////////////////////
#if 0	// MOVED TO SIOMAN.CPP

	int result;

	// Load the stack IRX file
#ifdef __NOPT_DEBUG__
	result = SIO::LoadIRX( "SNSTKDBG" );
#else
	result = SIO::LoadIRX( "SNSTKREL" );
#endif

    if( result < 0 )
    {
		Dbg_MsgAssert( 0,( "EE:Can't load module snstkrel/dbg.irx. Error : %d\n", result ));
		SetError( vRES_ERROR_INVALID_IRX );
		
		return false;
	}
#endif	
//////////////////////////////



#ifdef USE_DECI2
    // Load the DECI2 driver IRX file
    SIO::LoadIRX( "sndrv000" );
#else

	switch( GetConnectionType())
	{
		case vCONN_TYPE_PPPOE:
		{
			switch( GetDeviceType())
			{
				case vDEV_TYPE_USB_ETHERNET:
				{
					// Load the PPPoE Driver IRX file
					if( SIO::LoadIRX( "sndrv200" ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					// Load the USB-Ethernet Driver IRX file
					if( SIO::LoadIRX( "sndrv201" ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					break;
				}
				case vDEV_TYPE_PC_ETHERNET:
				{
					// Load the PPPoE Driver IRX file
					if( SIO::LoadIRX( "sndrv200" ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					// Load the SN Wrapper (PPPoE variant) for Sony Ether
					if( SIO::LoadIRX( "sndrv202" ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					} 
					// Load the Sony pcmcia irx
					if( SIO::LoadIRX( "dev9" ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					s_prepare_power_off();
					// Load the Sony Ethernet driver IRX file
					if( SIO::LoadIRX( "smap", 0, NULL, false ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					break;
				}
				default:
					SetError( vRES_ERROR_GENERAL );
					return false;
			}
			break;
		}
		case vCONN_TYPE_ETHERNET:
		{
			switch( GetDeviceType())
			{
				case vDEV_TYPE_USB_ETHERNET:
				{
					// Load the USB-Ethernet Driver IRX file
					if( SIO::LoadIRX( "sndrv001" ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					break;
				}
				case vDEV_TYPE_PC_ETHERNET:
				{
                    if( SIO::LoadIRX( "sndrv100" ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					// Load the Sony pcmcia irx
					if( SIO::LoadIRX( "dev9" ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					s_prepare_power_off();
					// Load the Sony Ethernet driver IRX file
					if( SIO::LoadIRX( "smap", 0, NULL, false ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					break;
				}
				default:
					SetError( vRES_ERROR_GENERAL );
					return false;
			}
			break;
		}
		case vCONN_TYPE_MODEM:
		{
			switch( GetDeviceType())
			{
				case vDEV_TYPE_USB_MODEM:
				{
					// Load the USB-Modem Driver IRX file
					if( SIO::LoadIRX( "sndrv002" ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					break;
				}

				case vDEV_TYPE_SONY_MODEM:
				{
					if (Config::PAL())
					{
						// Sony Modem Not Supported in PAL territories
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					
					// Load the SN modem wrapper irx
					if( SIO::LoadIRX( "sndrv101" ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					// Load the Sony pcmcia irx
					if( SIO::LoadIRX( "dev9" ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					s_prepare_power_off();
                    // Load the Sony modem driver irx
					if( SIO::LoadIRX( "spduart", sizeof(spduartArgs),  (char*) spduartArgs ) < 0 )
					{
						SetError( vRES_ERROR_DEVICE_NOT_CONNECTED );
						return false;
					}
					break;
				}
				default:
					SetError( vRES_ERROR_GENERAL );
					return false;
			}
			break;
		}
		default:
			// No valid device specified
			SetError( vRES_ERROR_GENERAL );
			return false;
	}
	
#endif	// USE_DECI2

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::initialize_ps2( void )
{
	
	
	bool result;

	Dbg_Printf( "initializing PS2\n" );
    
    if( m_net_drivers_loaded == false )
	{   
		bool success;

		File::StopStreaming( );
		if ( Pcm::UsingCD( ) )
		{
			Dbg_MsgAssert( 0,( "Can't load IRX modules when CD is busy." ));
			return false;
		}
	
		Dbg_Printf( "initializing PS2_2\n" );
		success = load_irx_files();
		m_net_drivers_loaded = true;
		if( success == false )
		{
			return false;
		}
	}
	else
	{
		if( m_device_changed )
		{
			SetError( vRES_ERROR_DEVICE_CHANGED );
			return false;
		}
	}
    
	result = initialize_device();
	if( result == false )
	{
		return false;
	}

	if( !m_stack_setup || m_options_changed )
	{
		m_stack_setup = sn_stack_setup();
	}

	m_device_changed = false;
	return m_stack_setup;
}

#endif	// __PLAT_NGPS__

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager::Manager( void )
{	
	int msg_id, i;

#if( defined ( __PLAT_WN32__ ) || defined ( __PLAT_XBOX__ ))
	int err;
	WORD version_required;
	WSADATA wsa_data;

#ifdef __PLAT_XBOX__
	XNetStartupParams xnsp;
	ZeroMemory( &xnsp, sizeof(xnsp) );
	xnsp.cfgSizeOfStruct = sizeof(xnsp);

#ifdef __NOPT_NOASSERTIONS__
	xnsp.cfgFlags = 0;
#else
	xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;// | XNET_STARTUP_BYPASS_DHCP;
#endif

	err = XNetStartup( &xnsp );
	if( err )
	{
		XNetCleanup();
		return;
	}	
#endif

	version_required = MAKEWORD( 2, 2 );
	if( err = WSAStartup ( version_required, &wsa_data ))
	{
		Dbg_MsgAssert( 0,( "Failed to start WinSock\n" ));
		WSACleanup();	
#ifdef __PLAT_XBOX__
		XNetCleanup();
#endif
		return;
	}	
	if (	( LOBYTE( wsa_data.wVersion ) != 2 ) ||
			( HIBYTE( wsa_data.wVersion ) != 2 ))
	{
		Dbg_MsgAssert( 0,( "Failed to start WinSock\n" ));
		WSACleanup();	
#ifdef __PLAT_XBOX__
		XNetCleanup();
#endif
		return;
	}

#ifdef __PLAT_XBOX__	
#	if 0
	HRESULT hr;
	XONLINE_STARTUP_PARAMS xosp = { 0 };

	hr = XOnlineStartup( &xosp );
	if( FAILED( hr ))
	{
		XOnlineCleanup();
		return;
	}
#	endif
#endif

#else    
#ifdef __PLAT_NGPS__
	ChangeThreadPriority( GetThreadId(), vMAIN_THREAD_PRIORITY );
	m_stack_setup = false;
	m_options_changed = false;
	m_device_changed = false;
	m_net_drivers_loaded = false;
#endif	// __PLAT_NGPS__
#endif	// __PLAT_WN32__

	for( msg_id = 0; msg_id < 255; msg_id++ )
	{
		SetMessageName( msg_id, "<No Text>" );
		SetMessageFlags( msg_id, 0 );
	}

	SetMessageName( MSG_ID_PING_TEST, "Ping Test" );
	SetMessageName( MSG_ID_PING_RESPONSE, "Ping Response" );
	SetMessageName( MSG_ID_CONNECTION_REQ, "Connection Request" );
	SetMessageName( MSG_ID_CONNECTION_ACCEPTED, "Connection Accepted" );
	SetMessageName( MSG_ID_CONNECTION_REFUSED, "Connection Refused" );
	SetMessageName( MSG_ID_CONNECTION_TERMINATED, "Connection Terminated" );
	SetMessageName( MSG_ID_SEQUENCED, "Sequenced Message" );
	SetMessageName( MSG_ID_ACK, "Ack" );
	SetMessageName( MSG_ID_FIND_SERVER, "Find Server" );
	SetMessageName( MSG_ID_SERVER_RESPONSE, "Server Find Response" );
	SetMessageName( MSG_ID_TIMESTAMP, "Timestamp" );
	SetMessageName( MSG_ID_ALIAS, "New Alias" );
	SetMessageName( MSG_ID_DISCONN_REQ,	"Disconn Request" );
	SetMessageName( MSG_ID_DISCONN_ACCEPTED, "Disconn Accepted" );

	SetMessageFlags( MSG_ID_TIMESTAMP, mMSG_SIZE_UNKNOWN );
	SetMessageFlags( MSG_ID_ACK, mMSG_SIZE_UNKNOWN );

	sprintf( m_local_ip, "<Unknown>" );
	sprintf( m_gateway, DEFAULT_SNPS2_GATEWAY );
	sprintf( m_subnet, DEFAULT_SNPS2_SUB_MSK );
			 
	m_num_apps = 0;
	m_conn_type = vCONN_TYPE_NONE;
	m_use_dhcp = false;
	m_online = false;
	m_use_dialup_auth = false;
	m_last_error = vRES_SUCCESS;
	m_modem_state = vMODEM_STATE_DISCONNECTED;
	m_modem_err = 0;

	for( i = 0; i < 3; i++ )
	{
		m_dns_servers[i][0] = '\0';
	}

#ifdef __PLAT_NGPS__
	m_bandwidth = 4200;	// Default to a 33.6kbps modem's approximate payload threshold (i.e. including packet overhead)
#else
	m_bandwidth = 400000;	// On Xbox, just assume broadband
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager::~Manager( void )
{
	
#if( defined ( __PLAT_WN32__ ) || defined ( __PLAT_XBOX__ ))
	WSACleanup();	
#endif

#ifdef __PLAT_XBOX__
	XNetCleanup();
#	if 0
	XOnlineCleanup();
#	endif
#endif

#ifdef __PLAT_NGC__
	SOCleanup();
#endif


#ifdef __PLAT_NGPS__
	sn_int32	stack_state, result;
	
	if( m_stack_setup )
	{
		// Stop the stack
		result = sn_stack_state(SN_STACK_STATE_STOP, &stack_state );
		Dbg_MsgAssert( result == 0,( "EE:sn_stack_sate() failed %d\n", result ));
			
		// De-Register this thread with the socket API
		Dbg_Printf( "EE:Calling sockAPIderegthr()\n" );
		sockAPIderegthr();
	}
#endif // __PLAY_NGPS__
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
#ifdef __PLAT_NGPS__

void Manager::conn_modem_state_callback( sn_int32 modem_state )
{
	
	Net::Manager * net_man = Net::Manager::Instance();
		
	Dbg_Printf("  Modem state %d = %s\n", modem_state, sntc_str_modem_state( modem_state ));
	switch( modem_state )
	{
		case SN_MODEM_READY:
		case SN_MODEM_DIALING:
			net_man->SetModemState( vMODEM_STATE_DIALING );
			
			// If we're using PAP/CHAP, clear the login script. We do it here instead of before
			// sntc_connect_modem() because that call resets to the default login script.
			if( net_man->ShouldUseDialupAuthentication())
			{
				sndev_set_null_scrpt_type clr;
				int result;

				clr.reserved = 0;

				result = sndev_set_options(0, SN_DEV_SET_NULL_SCRPT, &clr, sizeof(clr));
                Dbg_Printf( "EE:sndev_set_options(clr) returned %d\n", result );
			}
			
			Dbg_Printf( "Setting modem state to modem state dialing\n" );
			break;
		case SN_MODEM_LOGIN:
			Dbg_Printf( "Setting modem state to modem state connected\n" );
			net_man->SetModemState( vMODEM_STATE_CONNECTED );
			break;
		case SN_MODEM_PPP_UP:
			Dbg_Printf( "Setting modem state to modem state logged in\n" );
			net_man->SetModemState( vMODEM_STATE_LOGGED_IN );
			break;
		default:
			return;
	};
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void Manager::disconn_modem_state_callback( sn_int32 modem_state )
{
	Net::Manager * net_man = Net::Manager::Instance();
    
    Dbg_Printf("  Modem state = %s\n", sntc_str_modem_state( modem_state ));
	switch( modem_state )
	{
		case SN_MODEM_HANGINGUP:
			net_man->SetModemState( vMODEM_STATE_HANGING_UP );
			break;
		default:
			return;
	};
}

#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


bool	Manager::NetworkEnvironmentSetup( void )
{
#ifdef __PLAT_NGPS__
	return initialize_ps2();
#endif
#ifdef __PLAT_NGC__
	return initialize_ngc();
#endif

	return true;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::NeedToTestNetworkEnvironment( void )
{
#ifdef __PLAT_NGPS__
	if( m_net_drivers_loaded && m_device_changed )
	{
		return true;
	}
	if( !m_stack_setup || m_options_changed )
	{
		return true;
	}
#endif
	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ConnectToInternet( void )
{
	

#ifdef __PLAT_NGPS__

	if(( IsOnline() == false ) || m_options_changed )
	{
        if( GetConnectionType() == vCONN_TYPE_ETHERNET )
		{
			m_online = true;
		}
		else if(( GetConnectionType() == vCONN_TYPE_MODEM ) ||
				( GetConnectionType() == vCONN_TYPE_PPPOE ))
		{
			int result, device_type;
			int modem_state;
            
			s_cancel_dialup_conn = false;
            result = sndev_get_attached(0, &device_type, NULL, NULL);
			// Check that the above function call worked ok
			if( result != 0 )
			{
				SetModemState( vMODEM_STATE_ERROR );
				m_modem_err = SNTC_ERR_NOMODEM;
				return false;
			}
		
			// Check that there is a compatible modem attached
			if( device_type != SN_DEV_TYPE_USB_MODEM )
			{
				SetModemState( vMODEM_STATE_ERROR );
				m_modem_err = SNTC_ERR_NOMODEM;
				return false;
			}

			result = snmdm_get_state( &modem_state );
			// Check that the above function call worked ok
			if( result != 0 )
			{
				SetModemState( vMODEM_STATE_ERROR );
				m_modem_err = SNTC_ERR_NOMODEM;
				return false;
			}
		
			// Make sure that the modem is ready to dial, if not reset it
			if(	( modem_state != SN_MODEM_READY ) && 
				( modem_state != SN_MODEM_READY_AUTOANS ))
			{
				sn_char* err_msg_ptr;

				Dbg_Printf( "EE:Calling sntc_reset_modem()\n" );
				// Attempt to reset the modem
				result = sntc_reset_modem(	vMODEM_RESET_TIMEOUT,	// timeout_secs
											disconn_modem_state_callback,	// callback
											&err_msg_ptr );     	// error_message
				// If failed to reset the modem, then the result, and
				// error_message will have been set up by sntc_reset_modem.
				if( result != 0 )
				{
					SetModemState( vMODEM_STATE_ERROR );
					m_modem_err = SNTC_ERR_NOMODEM;
					return false;
				}
			}

			{
				struct SemaParam params;

				params.initCount = 1;
				params.maxCount = 10;
			
				s_conn_semaphore = CreateSema( &params );

                // Clear the modem state before we start
				SetModemState( -1 );

				m_modem_thread_data.m_pEntry = threaded_modem_conn;
				m_modem_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
				m_modem_thread_data.m_pStackBase = m_modem_thread_stack;
				m_modem_thread_data.m_iStackSize = vMODEM_THREAD_STACK_SIZE;
				m_modem_thread_data.m_utid = 0x150;//vBASE_SOCKET_THREAD_ID + NumApps();
				Thread::CreateThread( &m_modem_thread_data );
				m_modem_thread_id = m_modem_thread_data.m_osId;
                
				StartThread( m_modem_thread_id, this );
			}
		}   
	}
#else
	m_online = true;
#endif

	return m_online;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::DisconnectFromInternet( void )
{
	

#ifdef __PLAT_NGPS__
	if(	m_conn_type == vCONN_TYPE_ETHERNET )
	{
		m_online = false;
	}
	else if( ( m_conn_type == vCONN_TYPE_MODEM ) ||
			 ( m_conn_type == vCONN_TYPE_PPPOE ))
	{
        // Just in case the modem thread is running, stop it
		s_cancel_dialup_conn = true;
		WaitSema( s_conn_semaphore );
		DeleteSema( s_conn_semaphore );
		TerminateThread( m_modem_thread_id );
		DeleteThread( m_modem_thread_id );

		SetModemState( vMODEM_STATE_DISCONNECTING );
		m_modem_err = 0;

		m_modem_thread_data.m_pEntry = threaded_modem_disconn;
		m_modem_thread_data.m_iInitialPriority = vSOCKET_THREAD_PRIORITY;
		m_modem_thread_data.m_pStackBase = m_modem_thread_stack;
		m_modem_thread_data.m_iStackSize = vMODEM_THREAD_STACK_SIZE;
		m_modem_thread_data.m_utid = 0x14F;//vBASE_SOCKET_THREAD_ID + NumApps();
		Thread::CreateThread( &m_modem_thread_data );
		m_modem_thread_id = m_modem_thread_data.m_osId;
		
		StartThread( m_modem_thread_id, this );
	}
#else
	m_online = false;
#endif

	return ( m_online == false );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetModemBaudRate( void )
{
	return m_modem_baud_rate;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetBandwidth( int bytes_per_sec )
{
	m_bandwidth = bytes_per_sec;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetBandwidth( void )
{
	return m_bandwidth;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::IsOnline( void )
{   
#ifdef __PLAT_NGPS__
	if(	( GetConnectionType() == vCONN_TYPE_MODEM ) ||
		( GetConnectionType() == vCONN_TYPE_PPPOE ))
	{
		int modem_state;
        
		m_online = false;
		if( snmdm_get_state( &modem_state ) == 0 )
		{
			if( modem_state == SN_MODEM_PPP_UP )
			{
				m_online = true;
			}
		}
	}
#endif// __PLAT_NGPS__

	return m_online;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetISPPhoneNumber( char* phone_no )
{
	

	Dbg_Assert( phone_no );

	strcpy( m_isp_phone_no, phone_no );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetISPLogin( char* login )
{
	

	Dbg_Assert( login );

	strcpy( m_isp_user_name, login );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetISPPassword( char* password )
{
	

	Dbg_Assert( password );

	strcpy( m_isp_password, password );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetGateway( char* ip )
{
	

	if( strcmp( ip, m_gateway ))
	{
		strcpy( m_gateway, ip );
#ifdef __PLAT_NGPS__
		m_options_changed = true;
#endif		// __PLAT_NGC__
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetSubnetMask( char* ip )
{
	

	if( strcmp( ip, m_subnet ))
	{
		strcpy( m_subnet, ip );
#ifdef __PLAT_NGPS__
		m_options_changed = true;
#endif		// __PLAT_NGC__
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetPublicIP( unsigned int ip )
{
	m_public_ip = ip;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetLocalIP( char* ip )
{
	

	if( strcmp( ip, m_local_ip ))
	{
		strcpy( m_local_ip, ip );
#ifdef __PLAT_NGPS__
		m_options_changed = true;
#endif		// __PLAT_NGC__
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetConnectionType( ConnType conn_type )
{
	if( m_conn_type != conn_type )
	{
#ifdef __PLAT_NGPS__
		if( m_conn_type != vCONN_TYPE_NONE )
		{
			m_device_changed = true;
		}
#endif		// __PLAT_NGC__
		m_conn_type = conn_type;

        // Some default values for bandwidth limiting
		switch( m_conn_type )
		{
			case vCONN_TYPE_MODEM:
				SetBandwidth( 4200 );	
				break;
			case vCONN_TYPE_ETHERNET:
				SetBandwidth( 400000 );
				break;
			case vCONN_TYPE_PPPOE:
				SetBandwidth( 300000 );
				break;
			default:
				break;
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetDeviceType( DeviceType dev_type )
{
	if( m_device_type != dev_type )
	{
#ifdef __PLAT_NGPS__
		if( m_device_type != vDEV_TYPE_NONE )
		{
			m_device_changed = true;
		}
#endif		// __PLAT_NGC__
		m_device_type = dev_type;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetDHCP( bool use_dhcp )
{    
	if( m_use_dhcp != use_dhcp )
	{
		m_use_dhcp = use_dhcp;
#ifdef __PLAT_NGPS__
		m_options_changed = true;
#endif		// __PLAT_NGC__
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetDialupAuthentication( bool authenticate )
{
	m_use_dialup_auth = authenticate;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetDNSServer( int index, char* ip )
{
	
	
	Dbg_Assert(( index >= 0 ) && ( index < 3 ));
	Dbg_Assert( ip );

	if( strcmp( ip, m_dns_servers[index] ))
	{
		strcpy( m_dns_servers[index], ip );
#ifdef __PLAT_NGPS__
		m_options_changed = true;
#endif		// __PLAT_NGC__
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetHostName( char* host )
{
	

	// Either they pass us NULL or they pass us a value less than 32 chars long
	Dbg_Assert( !host || ( strlen( host ) < 32 ));

    if( host == NULL )
	{
		m_host_name[0] = '\0';
	}

	strcpy( m_host_name, host );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetDomainName( char* domain )
{
	

	// Either they pass us NULL or they pass us a value less than 32 chars long
	Dbg_Assert( !domain || ( strlen( domain ) < 32 ));

    if( domain == NULL )
	{
		m_domain_name[0] = '\0';
	}

	strcpy( m_domain_name, domain );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*	Manager::GetDNSServer( int index )
{
	

	Dbg_Assert(( index >= 0 ) && ( index < 3 ));

	return m_dns_servers[index];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*	Manager::GetISPPhoneNumber( void )
{
	if( GetConnectionType() == vCONN_TYPE_PPPOE )
	{
		return "";
	}
	return m_isp_phone_no;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ShouldUseDHCP( void )
{
	if(( GetConnectionType() == vCONN_TYPE_PPPOE ) ||
	   ( GetConnectionType() == vCONN_TYPE_MODEM ))
	{
		return false;
	}

	return m_use_dhcp;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::ShouldUseDialupAuthentication( void )
{
	if( GetConnectionType() == vCONN_TYPE_PPPOE )
	{
		return true;
	}
	return m_use_dialup_auth;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*	Manager::GetGateway( void )
{
	

	return m_gateway;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*	Manager::GetSubnetMask( void )
{
	

	return m_subnet;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*	Manager::GetHostName( void )
{
	

	return m_host_name;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*	Manager::GetDomainName( void )
{
	

	return m_domain_name;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

unsigned int	Manager::GetPublicIP( void )
{
	if( m_public_ip == 0 )
	{
		return inet_addr( m_local_ip );
	}
	
	return m_public_ip;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char*	Manager::GetLocalIP( void )
{   
	

#if defined( __PLAT_NGPS__ ) || defined( __PLAT_XBOX__ ) || defined( __PLAT_NGC__ )
	return m_local_ip;
#else
	struct hostent *host, *local_host;
	struct in_addr address;
	char *ip_str;
	int ip;

	if(( local_host = gethostbyname( "localhost" )))
	{	
		if(( host = gethostbyname( local_host->h_name )))
		{
			ip = *(unsigned long *) host->h_addr_list[0];			
			address.s_addr = ip;
			ip_str = inet_ntoa( address ); 
			return ip_str;
		}
		else
		{
			ip = *(unsigned long *) local_host->h_addr_list[0];			
			address.s_addr = ip;
			ip_str = inet_ntoa( address ); 
			return ip_str;
		}
	}
	
	return "<Unknown>";
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

ConnType	Manager::GetConnectionType( void )
{
	return m_conn_type;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

DeviceType	Manager::GetDeviceType( void )
{
#ifdef __PLAT_XBOX__
	return vDEV_TYPE_USB_ETHERNET;
#else
	return m_device_type;
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void		Manager::SetError( int error )
{
	m_last_error = error;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int			Manager::GetLastError( void )
{
	return m_last_error;
}

/******************************************************************/
/* Get the name associated with a message id 					  */
/*                                                                */
/******************************************************************/

char*	Manager::GetMessageName( unsigned char msg_id )
{
#ifdef	NET_DEBUG_MESSAGES
	return m_message_names[ msg_id ];
#else
	return "<Disabled>";
#endif
}

/******************************************************************/
/* Associate a text name with a message id for debugging purposes */
/*                                                                */
/******************************************************************/

void	Manager::SetMessageName( unsigned char msg_id, char* msg_name )
{
#ifdef NET_DEBUG_MESSAGES
	Dbg_Assert( msg_name );

	strncpy( m_message_names[ msg_id ], msg_name, vMAX_MSG_NAME_LEN - 1 );
	m_message_names[msg_id][vMAX_MSG_NAME_LEN - 1] = '\0';
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

char	Manager::GetMessageFlags( unsigned char msg_id )
{
	return m_message_flags[ msg_id ];
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetMessageFlags( unsigned char msg_id, char flags )
{
	m_message_flags[ msg_id ] = flags;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetModemState( void )
{
	return m_modem_state;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	Manager::SetModemState( int state )
{
	m_modem_state = state;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int		Manager::GetModemError( void )
{
	return m_modem_err;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool	Manager::CanChangeDevices( void )
{
#ifdef __PLAT_NGPS__
	return !m_net_drivers_loaded;
#else
	return true;
#endif
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

MsgLink::MsgLink( QueuedMsg *msg )
: Lst::Node< MsgLink > ( this ), m_QMsg( msg ), m_SendTime( 0 )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

MsgImpLink::MsgImpLink( QueuedMsg *msg )
: Lst::Node< MsgImpLink > ( this ), m_QMsg( msg ), m_SendTime( 0 )
{
	m_NumResends = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

MsgSeqLink::MsgSeqLink( QueuedMsgSeq *msg )
: Lst::Node< MsgSeqLink > ( this ), m_StreamMessage( 0 ), m_SendTime( 0 ), m_QMsg( msg )
{
	m_NumResends = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

StreamLink::StreamLink( StreamDesc* desc )
: Lst::Node< StreamLink > ( this ), m_Desc ( desc )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

StreamDesc::StreamDesc( void )
: m_Size( 0 ), m_Data( NULL ), m_DataPtr( NULL ), m_GroupId( 0 ), m_SendInPlace( false )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

StreamDesc::~StreamDesc( void )
{
	if( m_Data && !m_SendInPlace )
	{
		delete [] m_Data;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

MsgDesc::MsgDesc( void )
: m_Id( 0 ), m_StreamMessage( 0 ), m_Length( 0 ), m_Data( 0 ), m_Priority( NORMAL_PRIORITY ), m_Queue( QUEUE_DEFAULT ), m_GroupId( GROUP_ID_DEFAULT ),
	m_Singular( false ), m_Delay( 0 ), m_ForcedSequenceId( 0 )
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

QueuedMsg::QueuedMsg( unsigned char msg_id, unsigned short msg_len, void* data )
{
	
	
	m_Data = NULL;
	if( msg_len > 0 )
	{
		m_Data = new char[ msg_len ];
		memcpy( m_Data, data, msg_len );
	}
	
	m_MsgId = msg_id;
	m_MsgLength = msg_len;
#ifdef	__PLAT_NGPS__											  
	Dbg_MsgAssert(Mem::SameContext(this,Mem::Manager::sHandle().NetworkHeap()),("QueuedMsg not on network heap"));	
#endif		//	__PLAT_NGPS__											  
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

QueuedMsg::~QueuedMsg( void )
{
	
	if( m_Data )
	{
		delete [] m_Data;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

QueuedMsgSeq::QueuedMsgSeq( unsigned char msg_id, unsigned short msg_len, void* data, unsigned char group_id )
{
	
	
	m_Data = NULL;
	if( msg_len > 0 )
	{
		m_Data = new char[ msg_len ];
		memcpy( m_Data, data, msg_len );
	}
	
	m_MsgId = msg_id;
	m_MsgLength = msg_len;
	m_GroupId = group_id;	
#ifdef	__PLAT_NGPS__											  
	Dbg_MsgAssert(Mem::SameContext(this,Mem::Manager::sHandle().NetworkHeap()),("QueuedMsgSeq not on network heap"));	
#endif		//	__PLAT_NGPS__											  

}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

QueuedMsgSeq::~QueuedMsgSeq( void )
{
	if( m_Data )
	{
		delete [] m_Data;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

BitStream::BitStream( void )
{
	m_data = NULL;
	m_bits_left = 32;
	m_size = 0;
	m_cur_val = 0;
	m_bits_processed = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BitStream::SetInputData( char* data, int size )
{
	m_data = (unsigned int*) data;
	m_start_data = m_data;
	m_size = size;
	m_bits_left = 0;
	m_bits_processed = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BitStream::SetOutputData( char* data, int size )
{
	m_data = (unsigned int*) data;
	m_start_data = m_data;
	m_size = size;
	m_bits_left = 32;
	m_bits_processed = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	BitStream::GetByteLength( void )
{
	return (( m_bits_processed + 7 ) >> 3 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void	BitStream::WriteValue( int value, int num_bits )
{
	m_bits_processed += num_bits;
	
	// If we cannot fit all bits into our current int, we'll have to span it over
	// two ints
	if( m_bits_left < num_bits )
	{
		// Write out the first portion, then advance to the next int

        // First clear all top (unwritten) bits
		m_cur_val &= ( 1 << ( 32 - m_bits_left )) - 1;
		
		// Now write them (without having to mask low bits off)
		m_cur_val |= value << ( 32 - m_bits_left );
		num_bits -= m_bits_left;
		value >>= m_bits_left;
		memcpy( m_data, &m_cur_val, sizeof( unsigned int ));
		m_data++;
		m_cur_val = 0;
		m_bits_left = 32;
	}

    // First clear all top (unwritten) bits
	m_cur_val &= ( 1 << ( 32 - m_bits_left )) - 1;
	// Now write them (without having to mask low bits off)
	m_cur_val |= value << ( 32 - m_bits_left );
	m_bits_left -= num_bits;
	
	// Flush bits if full
	if( m_bits_left == 0 )
	{
		memcpy((char*) m_data, (char*) &m_cur_val, sizeof( unsigned int ));
		m_data++;
		m_cur_val = 0;
		m_bits_left = 32;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void BitStream::WriteFloatValue( float value )
{
	WriteValue( *(int*) &value, sizeof( float ) * 8 );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void BitStream::Flush( void )
{
	// Only flush if we have bits to flush
	if( m_bits_left != 32)
	{
		memcpy( m_data, &m_cur_val, sizeof( unsigned int ));
		m_data++;
		m_cur_val = 0;
		m_bits_left = 32;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

float			BitStream::ReadFloatValue( void )
{
	float res;
	int num_bits;
	
	num_bits = 32;
	m_bits_processed += num_bits;

	// If we're all out of bits (like at start), read a full word
	if( m_bits_left == 0 )
	{
		memcpy( &m_cur_val, m_data, sizeof( unsigned int ));
		m_data++;
		m_bits_left = 32;
	}
	
	// Do we hold all bits requested?
	if( m_bits_left >= num_bits )
	{
		// Yes, easy peasy case
		m_bits_left -= num_bits;
		if( num_bits == 32 )
		{
			memcpy( &res, &m_cur_val, sizeof( float ));
		}
		else
		{
			unsigned int int_val;

			int_val = m_cur_val & ((1 << num_bits) - 1);
			memcpy( &res, &int_val, sizeof( float ));
		}
		
		m_cur_val >>= num_bits;
	}
	else
	{
		unsigned int int_val;
		// Nope, grab those we have and fetch more from stream

		int_val = m_cur_val & (( 1 << m_bits_left ) - 1);
		num_bits -= m_bits_left;
		memcpy( &m_cur_val, m_data, sizeof( unsigned int ));
		m_data++;
		int_val |= ( m_cur_val & (( 1 << num_bits ) - 1)) << m_bits_left;
		m_cur_val >>= num_bits;
		m_bits_left = 32 - num_bits;

		memcpy( &res, &int_val, sizeof( float ));
	}
	
	return res;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

unsigned int	BitStream::ReadUnsignedValue( int num_bits )
{
	unsigned long res;
	
	Dbg_Assert(( num_bits >= 0 ) && ( num_bits <= 32 ));
   
	m_bits_processed += num_bits;

	// If we're all out of bits (like at start), read a full word
	if( m_bits_left == 0 )
	{
		memcpy( &m_cur_val, m_data, sizeof( unsigned int ));
		m_data++;
		m_bits_left = 32;
	}
	
	// Do we hold all bits requested?
	if( m_bits_left >= num_bits )
	{
				// Yes, easy peasy case
		m_bits_left -= num_bits;
		if( num_bits == 32 )
		{
			res = m_cur_val;
		}
		else
		{
			res = m_cur_val & ((1 << num_bits) - 1);
		}
		
		m_cur_val >>= num_bits;
	}
	else
	{
		// Nope, grab those we have and fetch more from stream
		res = m_cur_val & (( 1 << m_bits_left ) - 1);
		num_bits -= m_bits_left;
		memcpy( &m_cur_val, m_data, sizeof( unsigned int ));
		m_data++;
		res |= ( m_cur_val & (( 1 << num_bits ) - 1)) << m_bits_left;
		m_cur_val >>= num_bits;
		m_bits_left = 32 - num_bits;
	}
	
	return res;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int	BitStream::ReadSignedValue( int num_bits )
{
	long res;

	Dbg_Assert(( num_bits >= 0 ) && ( num_bits <= 32 ));

	// Cheasy call to that other function for simplicity
	res = ReadUnsignedValue( num_bits );

	// Sign extend result if sign bit set
	if( res & ( 1 << ( num_bits - 1 )))
	{
		res |= ~(( 1 << num_bits ) - 1 );
	}

	return res;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

}	// namespace Net