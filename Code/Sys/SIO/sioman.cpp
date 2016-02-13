/*****************************************************************************
**																			**
**			              Neversoft Entertainment			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SYS Library												**
**																			**
**	Module:			SYS (SYS_) 												**
**																			**
**	File name:		sioman.cpp												**
**																			**
**	Created:		05/26/2000	-	spg										**
**																			**
**	Description:	Serial IO Manager										**
**																			**
*****************************************************************************/


/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <string.h>
#include <sifdev.h>
#include <sifrpc.h>

#include <core/defines.h>

#include <sys/sioman.h>
#include <sys/siodev.h>
#include <sys/sio/keyboard.h>
#include <sys/file/filesys.h>
#include <sys/file/AsyncFilesys.h>
#include <sys/config/config.h>
#include <gel/module.h>

#include <libpad.h>
#include <libcdvd.h>
//#include <libmtap.h>

#include <libsdr.h>
#include <sdrcmd.h>

#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>

#include <gel/soundfx/soundfx.h>
#include <gel/soundfx/ngps/p_sfx.h>
#include <gel/music/music.h>

#include <dnas/dnas2.h>


/*****************************************************************************
**								DBG Information								**
*****************************************************************************/


       
namespace SIO
{



/*****************************************************************************
**								  Externals									**
*****************************************************************************/


/*****************************************************************************
**								Private Types								**
*****************************************************************************/


/*****************************************************************************
**								 Private Data								**
*****************************************************************************/

DefineSingletonClass( Manager, "Serial IO Manager" );

/*****************************************************************************
**								 Public Data								**
*****************************************************************************/



/*****************************************************************************
**							  Private Prototypes							**
*****************************************************************************/


/*****************************************************************************
**							   Private Functions							**
*****************************************************************************/

#define CD_TEST 0
#if CD_TEST
static void cd_test()
{
	int i;
	int	disk_type = SCECdDVD;

	sceCdInit(SCECdINIT);
	sceCdMmode(disk_type);
	printf(" sceCdGetDiskType   ");
	int detected_disk_type= sceCdGetDiskType();
	switch(detected_disk_type)
	{
		case SCECdIllgalMedia:
		printf("Disk Type= IllgalMedia\n"); break;
		case SCECdPS2DVD:
		printf("Disk Type= PlayStation2 DVD\n"); break;
		case SCECdPS2CD:
		printf("Disk Type= PlayStation2 CD\n"); break;
		case SCECdPS2CDDA:
		printf("Disk Type= PlayStation2 CD with CDDA\n"); break;
		case SCECdPSCD:
		printf("Disk Type= PlayStation CD\n"); break;
		case SCECdPSCDDA:
		printf("Disk Type= PlayStation CD with CDDA\n"); break;
		case SCECdDVDV:
		printf("Disk Type= DVD video\n"); break;
		case SCECdCDDA:
		printf("Disk Type= CD-DA\n"); break;
		case SCECdDETCT:
		printf("Working\n"); break;
		case SCECdNODISC: 
		printf("Disk Type= No Disc\n"); break;
		default:
		printf("Disk Type= OTHER DISK\n"); break;
	}

	// If disk type has changed to a CD, then need to re-initialize it							   
	if ((detected_disk_type == SCECdPS2CD) || (detected_disk_type == SCECdPSCD))
	{
		disk_type = SCECdCD;
		sceCdMmode(disk_type);
	}
	// This next bit is essential for when making a bootable disc, ie one that will
	// boot on the actual PS rather than just the dev system.
	// K: Commented out __NOPT_BOOTABLE__, since it is set when __NOPT_CDROM__OLD is set.

	/* Reboot IOP, replace default modules  */
	char  path[128];
	
	// THIS CODE IS NOT USED - JUST A TEST FUNCTION!!!!!
	sprintf(path,"host0:\\SKATE5\\DATA\\IOPMODULES\\DNAS280.IMG");	   // ALSO NEED TO CHANGE In p_filesys.cpp
	
	
	while ( !sceDNAS2NetSifRebootIop(path) ); /* (Important) Unlimited retries */
	while( !sceSifSyncIop() );

	/* Reinitialize */
	sceSifInitRpc(0);
	sceCdInit(SCECdINIT);

	sceCdMmode(disk_type);   /* Media: CD-ROM */

	sceFsReset();

	int fd = sceDopen("cdrom0:");
	if (fd >= 0)
	{
		struct sce_dirent dir_entry;
		Dbg_Message("Reading directory");
		while (sceDread(fd, &dir_entry) > 0)
		{
			Dbg_Message("Found entry %s", dir_entry.d_name);
		}
		sceDclose(fd);
	}
	else
	{
		// Couldn't open directory, try raw read
		uint32 buffer[512 + 16];
		for (i = 0; i < 512; i++)
		{
			buffer[i] = 0xDEADBEEF;
		}

		sceCdRMode mode;
		mode.trycount = 255;
		mode.spindlctrl = SCECdSpinNom;
		mode.datapattern = SCECdSecS2048;
		mode.pad = 0;

		int ret_val = sceCdRead(0, 1, buffer, &mode);
		if (ret_val == 1)
		{
			sceCdSync(0);
			Dbg_Message("Read Sector, here is the dump (make sure you don't see 0xDEADBEEF in data)");

			// Read data, print it
			for (i = 0; i < 512; i += 4)
			{
				Dbg_Message("0x%08x 0x%08x 0x%08x 0x%08x", buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3]);
			}
		}
		else
		{
			Dbg_Message("Couldn't Read Sector. Error = %x", sceCdGetError());
		}
	}
	Dbg_Assert(0);
}
#endif

void		Manager::process_devices( const Tsk::Task< Manager::DeviceList >& task )
{
	
//	Dbg_AssertType ( task, Tsk::Task< INP_MANAGER::DEVICE_LIST > );

	Device*					device;
	Lst::Search< Device >	    sh;
	Manager::DeviceList&	device_list = task.GetData();

	device = sh.FirstItem ( device_list );

	while ( device )
	{
		Dbg_AssertType ( device, Device );

		device->process();
		device = sh.NextItem();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Device* Manager::create_device( int index, int port, int slot )
{
    

    Device *device;
        
    device = new Device( index, port, slot );

    return device;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/
#define MAX_IRX_NAME_CHARS 100
static char pIRXNameBuf[MAX_IRX_NAME_CHARS+1];

// Loads an IRX file. The name must not have the .irx extension, it gets added by the function.
// Returns 0 if successful, -1 if error. (But it will assert in debug mode)
int LoadIRX(const char *pName, int num_args, char* args, bool assert_on_fail)
{
    
	
	File::StopStreaming( );
	if ( Pcm::UsingCD( ) )
	{
		Dbg_MsgAssert( 0,( "Using CD returned TRUE." ));
		return ( -1 );
	}

	// Start the name with the appropriate thingy.
	if (Config::CD())
	{
		Dbg_MsgAssert(strlen("cdrom0:\\xxxxxxxxxxxxxxxxxIOP\\")+strlen(pName)+strlen(".IRX")<=MAX_IRX_NAME_CHARS,("String in pName is too long."));
		char  path[128];
		sprintf(path,"cdrom0:\\%sIOP\\",Config::GetDirectory());
		strcpy(pIRXNameBuf,path);
	}
	else
	{
		Dbg_MsgAssert(strlen("host0:IOPModules/")+strlen(pName)+strlen(".IRX")<=MAX_IRX_NAME_CHARS,("String in pName is too long."));
		strcpy(pIRXNameBuf,"host0:IOPModules/");
	}	
	
	// Append the name in upper case.
	char *pDest=pIRXNameBuf+strlen(pIRXNameBuf);
	const char *pSource=pName;
	while (*pSource)
	{
		char ch=*pSource++;
		if (ch>='a' && ch<='z')
		{
			ch='A'+ch-'a';
		}	
		*pDest++=ch;
	}
	*pDest=0;
	
	// Append the extension.
	strcat(pIRXNameBuf,".IRX");
	
	int result;
	if(( result = sceSifLoadModule(pIRXNameBuf, num_args, args )) < 0 )
	{
		Dbg_MsgAssert( !assert_on_fail,( "Can't load module %s. Result: %d\n",pIRXNameBuf, result ));
		
		return result;
	}   
	return 0;		
}

Manager::Manager( void )
{
    

    int i, j, index;
    Device *device;
    
    sceSifInitRpc(0);

#if CD_TEST
	cd_test();
#endif

	LoadIRX("sio2man");
#ifdef MULTI_TAP_SUPPORT
	LoadIRX("mtapman");
#endif // MULTI_TAP_SUPPORT
	LoadIRX("mcman");
	LoadIRX("mcserv");
	LoadIRX("padman");
	LoadIRX("cdvdstm");
    
	// Moved the loading of the SN Stack module here from net.cpp because it seems to use some
	// temporary memory upon startup that causes it to exceed the IOP memory limit if loaded
	// last.
#ifdef __NOPT_DEBUG__
	LoadIRX( "SNSTKDBG" );
#else
	LoadIRX( "SNSTKREL" );
#endif

	if (!Sfx::NoSoundPlease() || !Pcm::NoMusicPlease() || !Pcm::StreamsDisabled())
	{
		LoadIRX("libsd");	// sound chip SPU2 lib
		LoadIRX("sdrdrv");	// sound driver lib?
	}

	// Async file driver
	LoadIRX("fileio");
	// Init the new async stuff
	File::CAsyncFileLoader::sInit();

	if (!Sfx::NoSoundPlease() || !Pcm::NoMusicPlease() || !Pcm::StreamsDisabled())
	{
		LoadIRX("ezpcm");	// streaming music...
	}

	LoadIRX("usbd");		// usb driver

	#define ICON_FILE	"foo"	//"mc0:BASLUS-20731/NAAPROIA/SYS_NET.ICO"// "foo"
	#define ICON_SYS_FILE	"bar"	//"mc0:BASLUS-20731/NAAPROIA/icon.sys"// "bar"

	static char netcnfArgs[] = "icon="ICON_FILE"\0iconsys="ICON_SYS_FILE;

	LoadIRX("netcnf", sizeof(netcnfArgs), netcnfArgs );

#if ( KEYBOARD_ON )
	// initialize the keyboard
	KeyboardInit();
#endif

	// initialise IOP memory ( used for CD filesystem and for music ).
	if( sceSifInitIopHeap( ) < 0 )
	{
		Dbg_MsgAssert( 0,( "Failed to init IOP Heap\n"));
	}

    if( scePadInit( 0 ) != 1 )
    {
        Dbg_MsgAssert( false,( "failed to init IOP controller lib\n" ));
    }
                         
#ifdef MULTI_TAP_SUPPORT
    sceMtapInit();
	sceMtapPortOpen( 0 );
    if( sceMtapGetConnection( 0 ) != 1 )
	{
		sceMtapPortClose( 0 );
	}
#endif // MULTI_TAP_SUPPORT

    m_process_devices_task = new Tsk::Task< DeviceList > ( Manager::process_devices, m_devices );
    
    index = 0;
    for( i = 0; i < vMAX_PORT; i++ )
    {
       for( j = 0; j < scePadGetSlotMax ( i ); j++ )
       {
           if(( device = create_device ( index, i, j )))
           {
               m_devices.AddToTail( device->m_node );
               device->Acquire();
               index++;
           }
       }
    }
	
	if (!Sfx::NoSoundPlease())
	{
		Sfx::PS2Sfx_InitCold( );
	}
	
	if (!Pcm::NoMusicPlease() || !Pcm::StreamsDisabled())
	{
		Pcm::Init( );
	}

	if (Config::CD())
	{
		File::InitQuickFileSystem( );
	}	
    Dbg_Message( "Initialized IOP controller lib\n" );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Manager::~Manager( void )
{
    

    Device*					device;
    Device*					next;
    Lst::Search< Device >	sh;
    
    device = sh.FirstItem ( m_devices );
    
    while ( device )
    {
        Dbg_AssertType ( device, Device );
    
        next = sh.NextItem();
    
        delete device;
        device = next;
    }

    delete m_process_devices_task;

    if( scePadEnd() == 1 )
    {
        Dbg_Message( "Shut down IOP Controller Lib\n" );
    }
    else
    {
        Dbg_MsgAssert( false,( "Failed to shut down IOP Controller Lib\n" ));
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Device* Manager::GetDevice( int port, int slot )
{
    

    Device*					device;
    Lst::Search< Device >	sh;
    
    for( device = sh.FirstItem ( m_devices ); device;
            device = sh.NextItem ())
    {
        if( ( device->GetPort() == port ) &&
            ( device->GetSlot() == slot ))
        {
            return device;
        }
    }
    
    return NULL;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

Device* Manager::GetDeviceByIndex( int index )
{
    

    Device*					device;
    Lst::Search< Device >	sh;
    
    for( device = sh.FirstItem ( m_devices ); device;
            device = sh.NextItem ())
    {
        if( device->GetIndex() == index )
        {
            return device;
        }
    }
    
    return NULL;
}

/******************************************************************/
/*                                                                */
/*  Ken: Runs through all the devices pausing each one.           */
/*                                                                */
/******************************************************************/

void Manager::Pause()
{
    

    Device*					device;
    Lst::Search< Device >	sh;
    
    for( device = sh.FirstItem ( m_devices ); device;
            device = sh.NextItem ())
    {
		device->Pause();
    }
}

/******************************************************************/
/*                                                                */
/*  Ken: Runs through all the devices unpausing each one.         */
/*                                                                */
/******************************************************************/

void Manager::UnPause()
{
    

    Device*					device;
    Lst::Search< Device >	sh;
    
    for( device = sh.FirstItem ( m_devices ); device;
            device = sh.NextItem ())
    {
		device->UnPause();
    }
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace SIO
