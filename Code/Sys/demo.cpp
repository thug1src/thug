//****************************************************************************
//* MODULE:         sys
//* FILENAME:       Demo.cpp
//* OWNER:          Gary Jesdanun
//* CREATION DATE:  10/16/2002
//****************************************************************************

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <sys/demo.h>

#include <core/defines.h>

#include <gel/music/music.h>
#include <gel/soundfx/soundfx.h>

#ifdef __PLAT_NGPS__
    #include <eekernel.h>
    #include <eetypes.h>
    #include <libcdvd.h>
    #include <libpad.h>

    #include <libscedemo.h>
    extern	sceDemoEndReason demo_exit_reason;
    extern "C" void sceSifExitCmd(void);

    #include <gfx/ngps/nx/nx_init.h>
#endif // __PLAT_NGPS__

#include <sys/config/config.h>

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

namespace Sys
{

/*****************************************************************************
**								   Externals								**
*****************************************************************************/

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

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
**							   Private Functions							**
*****************************************************************************/

#ifdef __PLAT_NGPS__

void prepare_to_exit()
{
	printf ("******  Exiting THPS Demo *****\n");

	//Nx::CEngine::sSuspendEngine();		// turn off Nx engine interrupts
	NxPs2::SuspendEngine();				// 
	
	Sfx::CSfxManager* sfx_manager =  Sfx::CSfxManager::Instance();
	sfx_manager->PauseSounds( );
	if (!Config::CD())
	{
		printf ("Re-initializing the CD/DVD drive\n");
		// if we did not come from a CD, then we need to initilaize the CD here
		// so we can test exiting back to the bootstrap program
	    sceCdInit(SCECdINIT);
	    sceCdMmode(SCECdDVD);
	    sceCdDiskReady(0);
	}
	else
	{
		printf ("Stopping streaming etc, on CD/DVD drive\n");
		Pcm::StopStreams(  );
		Pcm::StopMusic(  );
		sceCdStStop();				// also stop streaming....	   Yes!!
		sceCdStop();				// stop Cd
		sceCdSync(0);				// wait for commands to execute
		sceCdDiskReady(0);		   	// wait for Cd to be ready again
	}	

	scePadEnd();				// Another Sony reccomendation (from James Wang, newgroup post, 6/11/2002)

}
#endif

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool LoadExec( const char* pELFName )
{

#ifdef __PLAT_NGPS__
	char p_string[100];
	sprintf(p_string,"%s;1",pELFName);	   		// add on the ;1

	prepare_to_exit();
		
	sceSifExitCmd();			// Sony suggested fix to make LoadExecPS2 work more reliably. 
    
	char *args[4];
	args[0] =  "bootstrap";
	if (Config::NTSC())
	{
		args[1] =  "cdrom0:\\SLUS_207.31;1";
		args[2] = "";
		args[3] = "";
	}
	else
	{	
		switch (Config::GetLanguage())
		{
			case Config::LANGUAGE_FRENCH:
				args[1] =  "cdrom0:\\SLES_518.51;1";
				break;
			case Config::LANGUAGE_GERMAN:
				args[1] =  "cdrom0:\\SLES_518.52;1";
				break;
			case Config::LANGUAGE_ITALIAN:
				args[1] =  "cdrom0:\\SLES_518.53;1";
				break;
			case Config::LANGUAGE_SPANISH:
				args[1] =  "cdrom0:\\SLES_518.54;1";
				break;
			default:
				Dbg_MsgAssert(0,("Bad Language (%d)",(int)(Config::GetLanguage())));
				// allow drop through to english as a failsafe.  
			case Config::LANGUAGE_ENGLISH:
				args[1] =  "cdrom0:\\SLES_518.48;1";
				break;			
		}
		args[2] = "PAL";
		args[3] = "Framerate=50";
	}
    printf ("LoadExecPS2(%s) args = (%s,%s,%s,%s)\n",p_string, args[0], args[1], args[2], args[3] );
	LoadExecPS2(p_string, 4, args);
	printf ("LoadExecPS2 Failed - probably file not found, profiling on, or bad media type\n");
//    LoadExecPS2("cdrom0:\\demo\\slus_201.99;1", 2, args);
   

#endif		// __PLAT_NGC__
	return true;		// actually will never get here}
}


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

/*
- TODO - factor out the common functionality of ExitDemo into LoadExecPS2

the LoadExecPS2() references lists some common gotchas:

The total size of the character strings for the filename and all of the
arguments must not exceed 256 bytes. 

Note the following points before calling LoadExecPS2(). 
* Terminate or delete all threads other than the thread that will 
  execute LoadExecPS2(). 
* Cancel all callbacks and interrupt handlers. 
* Execute scePadEnd(). 
* Execute sceSifExitCmd().
*/

bool ExitDemo()
{

#ifdef	__PLAT_NGPS__													  

	prepare_to_exit();													  

	while (1)
	{		
		sceSifExitCmd();			// Sony suggested fix to make LoadExecPS2 work more reliably. 
	
		if (Config::SonyBootstrap())
		{
			printf ("Exiting with sceDemoEnd\n");
			sceDemoEnd(demo_exit_reason);
		}
		else
		{
			sceSifExitCmd();			// Sony suggested fix to make LoadExecPS2 work more reliably. 
			char *args[] = {Config::gReturnString };
			printf ("Exiting with LoadExecPS2( %s [%s] )\n", Config::gReturnTo, Config::gReturnString); 
			LoadExecPS2(Config::gReturnTo, 1, args);
		}
		printf ("ERROR - Failed to exit!!!!!!! RETRYING\n");
	}		
#endif
   
	return true;		// actually will never get here
}

} // namespace Sys
