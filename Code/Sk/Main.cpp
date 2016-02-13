/*****************************************************************************
**				   															**
**			              Neversoft Entertainment.			                **
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		PS2														**
**																			**
**	Module:			Main		 											**
**																			**
**	File name:		main.cpp												**
**																			**
**	Created:		a long long time ago									**
**																			**
**	Description:	Main entry code											**
**																			**
*****************************************************************************/

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <stdio.h>
#include <string.h>


extern int maintest(int argc, char *argv[]);
extern void test_init();
extern void test_render();

#ifdef __PLAT_NGC__
#ifdef DVDETH
#include <dolphin/dvdeth.h>
#endif
#include <libsn.h>
#endif

#include <core/defines.h>	
#include <core/thread.h>
#include <core/singleton.h>
		 
#include <sys/profiler.h>
#include <sys/timer.h>
#include <sys/mem/memman.h>
#include <sys/mem/memtest.h>
#include <sys/sioman.h>										   
#include <sys/File/PRE.h>
#include <sys/File/filesys.h>
#include <sys/File/AsyncFilesys.h>
#include <sys/mcman.h>
#include <sys/config/config.h>


#include <gfx/gfxman.h>
#include <gfx/nx.h>
#include <gfx/nxloadscreen.h>
#include <gfx/ngps/nx/nx_init.h>


#include <gel/modman.h>
#include <gel/mainloop.h>
#include <gel/objman.h>
#include <gel/objtrack.h>
#include <gel/inpman.h>
#include <gel/soundfx/soundfx.h>
#include <gel/music/music.h>
#include <gel/net/net.h>
#include <gel/assman/assman.h>
#include <sk/modules/skate/skate.h>
#include <sk/modules/FrontEnd/FrontEnd.h>
#include <sk/objects/movingobject.h>
#include <sk/gamenet/gamenet.h>
#include <gel/scripting/scriptcache.h> 
#include <gel/scripting/script.h> 
#include <gel/scripting/parse.h> 
#include <gel/scripting/symboltable.h> 
#include <sk/scripting/gs_init.h>
#include <gfx/2D/ScreenElemMan.h>
#include <sys/mem/PoolManager.h>
#include <objects/pathman.h>
#include <sys/replay/replay.h>
#include <gel/object/compositeobjectmanager.h>

#include <sk/ParkEditor2/ParkEd.h>
#include <gel/scripting/Debugger.h>

#include <gfx/ngps/nx/pcrtc.h>

#ifdef __PLAT_NGPS__
#include <libgraph.h>
#endif

#ifdef __PLAT_NGC__
#include <sys/ngc/p_display.h>
#endif		// __PLAT_NGC__

#define MAINLOOP_MEMMARKER 16

#ifdef	__PLAT_NGPS__
#include <eeregs.h>
#include <libscedemo.h>
extern char _code_start[];
extern char _code_end[];

extern char __floatdisf[];
extern char dptofp[];
extern char __fixunssfdi[];
extern char __udivdi3[];
#endif



#ifdef	__PLAT_NGPS__
sceDemoEndReason		demo_exit_reason = SCE_DEMO_ENDREASON_PLAYABLE_QUIT;	// default reason is that we quit (timeouts overrride this, and we can't "complete")
#define	DEBUG_FLASH(x)  //*GS_BGCOLOR = x  // 0xBBGGRR
#else
#define	DEBUG_FLASH(x)
#endif

int		inactive_time = 0;
int		inactive_countdown = 0;
int		gameplay_time = 0;



bool	skip_startup = false;

/*****************************************************************************
**								DBG Information								**
*****************************************************************************/

Dbg_DefineProject( PS2, "Test Project" )

/*****************************************************************************
**								  Externals									**
*****************************************************************************/

extern "C"
{
#ifdef __PLAT_XBOX__
int pre_main( void );
#endif
#ifdef __PLAT_NGC__
void __init_vm(void);
#endif
#ifdef __PLAT_NGPS__
void pre_main( void );
#endif
void post_main( void );
}

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

/*****************************************************************************
**							   Public Functions								**
*****************************************************************************/

#ifdef __PLAT_XBOX__
int pre_main( void )
#endif
#ifdef __PLAT_NGC__
void __init_vm( void )
#endif
#ifdef __PLAT_NGPS__
void pre_main( void )
#endif
{
	DEBUG_FLASH(0x07f7f7f);		// initial white


#ifdef __PLAT_NGPS__
	printf ("calling Mike test\n");
//	maintest(0, NULL); // the Mike code.....
	printf ("DONE calling Mike test\n");
#endif		// __PLAT_NGPS__

#ifdef __PLAT_NGC__
	OSInitFastCast();
	NsDisplay::init();
#endif		// __PLAT_NGC__

	Dbg::SetUp();
	Mem::Manager::sSetUp();
	
	DEBUG_FLASH(0x02050);		// brown

#ifdef __PLAT_XBOX__
	// Must return 0 here, as this is called from CRT initialization code.
	return 0;
}

#pragma data_seg( ".CRT$RIX" )
static int (*_mypreinit)(void) = pre_main;
#pragma data_seg()

#else
}
#endif

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void post_main (void)
{
	Mem::Manager::sCloseDown();
	Dbg::CloseDown();
}

void mat_test(Mth::Matrix A,Mth::Matrix B,Mth::Matrix C);

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


int main ( sint argc, char** argv )
{ 	
#ifdef __PLAT_NGC__
#ifdef DVDETH
	// Defaults
    u8   DvdServerAddr[4]    = { 192, 168,   0,   153 };// PC IP Address
    u16  DvdServerPort       = 9001;                  // PC TCP PORT

//    u8   ReportServerAddr[4] = { 192, 168,   0,   153 };// PC IP Address
//    u16  ReportServerPort    = 9002;                  // PC TCP PORT

    u8   GCClientAddr[4]     = { 192, 168,   5,   153 };// GC IP Adderss
    u8   Netmask[4]          = { 255, 255,   0,   0 };// Netmask
    u8   Gateway[4]          = { 192, 168,   0, 254 };// Gateway
#endif		// DVDETH


	// Add -a -pf to odemrun to enable printfs.
	bool kill = true;
	for ( int arg = 1; arg < argc; arg++ )
	{
		if ( argv[arg][0] != '-' ) continue;

		switch ( argv[arg][1] )
		{
			case 'p':
				// Kill printfs.
				if ( argv[arg][2] == 'f' )
				{
					kill = false;
				}
				break;
#ifdef DVDETH
			case 'i':
				// specify IP address.
				switch ( argv[arg][2] )
				{
					case 'p':
						{
							// PC IP address
							char num0[8];
							char num1[8];
							char num2[8];
							char num3[8];
							int c = 0;
							int d;

							// Get the numbers individually.
							d = 0;      while ( argv[arg][3+c] != '.' ) { num0[d++] = argv[arg][3+c++]; } num0[d] = '\0';
							d = 0; c++; while ( argv[arg][3+c] != '.' ) { num1[d++] = argv[arg][3+c++]; } num1[d] = '\0';
							d = 0; c++; while ( argv[arg][3+c] != '.' ) { num2[d++] = argv[arg][3+c++]; } num2[d] = '\0';
							d = 0; c++; while ( argv[arg][3+c] != '\0' ) { num3[d++] = argv[arg][3+c++]; } num3[d] = '\0';

							DvdServerAddr[0] = atoi( num0 );
							DvdServerAddr[1] = atoi( num1 );
							DvdServerAddr[2] = atoi( num2 );
							DvdServerAddr[3] = atoi( num3 );

//							ReportServerAddr[0] = atoi( num0 );
//							ReportServerAddr[1] = atoi( num1 );
//							ReportServerAddr[2] = atoi( num2 );
//							ReportServerAddr[3] = atoi( num3 );
						}
						break;
					case 'g':
						{
							// GameCube IP address
							char num0[8];
							char num1[8];
							char num2[8];
							char num3[8];
							int c = 0;
							int d;

							// Get the numbers individually.
							d = 0;      while ( argv[arg][3+c] != '.' ) { num0[d++] = argv[arg][3+c++]; } num0[d] = '\0';
							d = 0; c++; while ( argv[arg][3+c] != '.' ) { num1[d++] = argv[arg][3+c++]; } num1[d] = '\0';
							d = 0; c++; while ( argv[arg][3+c] != '.' ) { num2[d++] = argv[arg][3+c++]; } num2[d] = '\0';
							d = 0; c++; while ( argv[arg][3+c] != '\0' ) { num3[d++] = argv[arg][3+c++]; } num3[d] = '\0';

							GCClientAddr[0] = atoi( num0 );
							GCClientAddr[1] = atoi( num1 );
							GCClientAddr[2] = atoi( num2 );
							GCClientAddr[3] = atoi( num3 );
						}
						break;
					default:
						break;
				}
				break;
#endif		// DVDETH
			default:
				break;
		}
	}

#	if !defined( __PLAT_NGC__ ) || ( defined( __PLAT_NGC__ ) && !defined( __NOPT_FINAL__ ) )
	if ( kill )
	{
		// Kill printfs
		uint32 * p;
		p = (uint32*)&OurPrintf;
		*p = 0x4e800020;		// blr
		p = (uint32*)&OSReport;
		*p = 0x4e800020;        // blr
	}
#endif	// __NOPT_FINAL__

#ifdef DVDETH

    // Initialize Network
    DVDEthInit(GCClientAddr, Netmask, Gateway);

    // Initialize DVD
    DVDLowInit(DvdServerAddr, DvdServerPort);

//    // Initialize OSReport();
//    OSReportInit(ReportServerAddr, ReportServerPort);
#endif

#endif		// __PLAT_NGC__

	// Note: printf's will not work until after Config::Init() is called.

	DEBUG_FLASH(0x007F0000);
	
	Mem::SetThreadSafe(false);	
	Config::Init(argc,argv);

#ifdef __PLAT_NGPS__
	// a hack to disable the PS2 video output as early as poss
	NxPs2::SetupPCRTC(0, SCE_GS_FIELD);
#endif		// __PLAT_NGPS__

	DEBUG_FLASH(0x00007f00);
					  
	Mem::Manager::sHandle().InitOtherHeaps();							

	DEBUG_FLASH(0x0000007f);

	#ifdef	__PLAT_NGPS__
// Need to reconcile the Config::Init with the following code
// so demo version can exist with regular version
// just switched by examining command line

	if (Config::SonyBootstrap())
	{
								   
		unsigned short language;
		unsigned short aspect;
		unsigned short playmode;
		unsigned short to_inactive;
		unsigned short to_total;
		unsigned short mediaType;
		unsigned short masterVolumeScale = 10;
		unsigned short directorySectorNum;
			
		sceDemoStart(argc,argv,&language,&aspect,&playmode,&to_inactive,&to_total,&mediaType,&masterVolumeScale,&directorySectorNum);
		
		printf("Tony Hawk's Pro Skater 4 - Demo Disk Version\n");
		printf("l %d a %d pm %d toa %d tot %d\n",language,aspect,playmode,to_inactive,to_total);
		printf("MEDIA TYPE %d\n",mediaType);
		printf("MasterVol Scale %d\n",masterVolumeScale);
		printf("My directory Sector is %d\n",directorySectorNum);
		
		inactive_time = to_inactive * 60;
		inactive_countdown = to_inactive * 60;
		gameplay_time = to_total * 60;
		
		if (Config::Bootstrap() && Config::CD())
		{
			Config::SetMasterVolume(masterVolumeScale * 10.0f);
		}
	}
	#endif		   
	
	
//	Config::SetMasterVolume(0.0f);
	 		   
	
	
	printf ("\n Start of main()\n");
	printf ("argc = %d\n",argc); 
	for (int arg = 0; arg < argc; arg++)
	{
		printf ("%2d: %s\n",arg,argv[arg]);
	}
	printf ("\n");
	
	//printf("Hardware=%s\n",Config::GetHardwareName());
	//printf("GotExtraMemory=%d\n",Config::GotExtraMemory());
	//printf("Language=%s\n",Config::GetLanguageName());
	//printf("CD=%d\n",Config::CD());
	
	if (Config::GotExtraMemory())
	{
		Mem::Manager::sSetUpDebugHeap();
	}	

	Mem::PushMemProfile("Everything");
	Mem::PushMemProfile("Initialization");

	
//	static char	profdata[8192];
//	snDebugInit();
//	snProfInit(_4KHZ, profdata, sizeof(profdata));

	if (argc == 2 && strcmp(argv[1],"demo") == 0)
	{
		skip_startup = true;
	}
	
	
	DEBUG_FLASH(0x007f007f);		// magenta
	


#ifdef	__NOPT_ASSERT__
#ifdef	__PLAT_NGPS__	
	static u_long128 profdata[2048]; // quadword aligned, can be 2K
									 // to 64K bytes

	// we now always turn on profiling if running on a TOOL (unless we are on a CD)									 
	if (Config::GetHardware() == Config::HARDWARE_PS2_DEVSYSTEM && !Config::CD())
	{
		snDebugInit();
		sceSifInitRpc(0);
		// Load the SNProfil module
		if(sceSifLoadModule("host0:/usr/local/sce/iop/modules/SNProfil.irx", 0, NULL) < 0)
		{
			printf("Can't load SNProfil module, but don't worry, that's just for programmers...\n However, you could get the latest version of usr if you want it\n");
	//		exit(-1);
		}
		else
		{
			if(snProfInit(_4KHZ, profdata, sizeof(profdata)) != 0)
			{
				printf("Profiler init failed\n"); // see SN_PRF… in LIBSN.H
			}
		}	
	}	
	// rest of user code follows on from here...
#endif
#endif

	Dbg_Message ( "Begin Application" );						 
	Mth::InitialRand(107482099);

#if 0
		Mth::InitSinLookupTable();
#ifdef  __PLAT_NGPS__
// (Mick)  THis might not be needed with the new libs
													   
		// Run through all the code and replace all calls to sinf, cosf
		// with faster versions that use a lookup table.
		// Note: This must be done after the call to Mth::InitSinLookupTable(); above,
		// because that uses the original sinf to generate the lookup table.
		int *pc = (int*)_code_start;
		while (pc < (int*)_code_end)
		{

			int instruction = *pc;
			if (instruction >> 24 == 0x0c)
			{
				int code = (instruction & 0xffffff)<<2;
				if (code == (int)sinf)
				{									
					*pc=(*pc&0xff000000)| ((((uint32)Mth::Kensinf)>>2)&0xffffff);
				}
				else if (code == (int)cosf)
				{
					*pc=(*pc&0xff000000)| ((((uint32)Mth::Kencosf)>>2)&0xffffff);
				}
				else if (code == (int)acosf)
				{
					*pc=(*pc&0xff000000)| ((((uint32)Mth::Kenacosf)>>2)&0xffffff);
				}
			}
			++pc;
	   }
	   
#endif				 
#endif				 


#ifdef __NOPT_ASSERT__
#ifdef  __PLAT_NGPS__
	Mem::PushMemProfile("(debug only) Log::Init()");
	Log::Init();
	Mem::PopMemProfile();
#endif
#endif

	
	while (  true )
	{	
		Mem::Manager::sHandle().BottomUpHeap()->PushContext();
		Mem::Manager::sHandle().PushMemoryMarker(MAINLOOP_MEMMARKER);
		{
			/*
				Important Notes:
				
				Please put all module creation/registering/starting code in its proper section.
				This helps prevents bugs caused by initialization code depending on objects
				that have not been initialized.
				
				There are two stages to initialization. First, object creation (create singleton
				section). Second, running the object's v_start function (start modules section).
				Note that the latter doesn't happen when StartModule is called, but after the
				main loop is entered.
			
				It would be a bad idea to make code in any module's constructor depend on initialization 
				done in another's v_start function.
			*/
			
			// We create the symbol hash table early, as it's kind of used my Music::Init, which I think
			// is called both at startup (when the scripts are invalid), and later (when they are valid)
			Script::CreateSymbolHashTable();
								
			
			/***********************************************
			General startup section
			************************************************/
			
			Mem::PushMemProfile("Hash Item Pool Manager");
#			if( defined( __PLAT_NGC__ ) || defined( __PLAT_XBOX__ ))
			Mem::PoolManager::SSetupPool(Mem::PoolManager::vHASH_ITEM_POOL, 12500);	// Mick: increased from 5600 to 10000
#			else
			Mem::PoolManager::SSetupPool(Mem::PoolManager::vHASH_ITEM_POOL, 10000);	// Mick: increased from 5600 to 10000
#			endif // __PLAT_NGC__ || __PLAT_XBOX__
			Mem::PopMemProfile(/*"Hash Item Pool Manager"*/);

				
			// I'd prefer for this stuff to happen after the singleton section,
			// but it won't work that way
			
			Spt::SingletonPtr< File::PreMgr> pre_mgr(true);		 // Note, moved here so SkateScript::Init can use it
			   	
			/***********************************************
			Create singleton section
			************************************************/
			
			/*
				RJM:
				
				-SkateMod must come after panelMgr, because it references the panelMgr singleton
			*/
			
			Tmr::Init();
			
			Mem::PushMemProfile("File System");
			File::InstallFileSystem();               
			Mem::PopMemProfile(/*"File System"*/);

								
			DEBUG_FLASH(0x007f7f00);		// cyan
								
			Mem::PushMemProfile("System Singletons");
			Spt::SingletonPtr< Obj::CTracker >	obj_tracker( true );
			Spt::SingletonPtr< Mlp::Manager >	mlp_manager( true );
			Spt::SingletonPtr< Mdl::Manager >	mdl_manager( true );
			Spt::SingletonPtr< SIO::Manager >	sio_manager( true );
			Spt::SingletonPtr< Mc::Manager > 	mc_manager( true );
			Spt::SingletonPtr< Inp::Manager >	inp_manager( true );
			Spt::SingletonPtr< Gfx::Manager >	gfx_manager( true );
			Spt::SingletonPtr< File::CAsyncFilePoll>	async_poll( true );
			Mem::PopMemProfile(/*"System Singletons"*/);

			DEBUG_FLASH(0x00007f7f);		// yellow


			#ifdef	__PLAT_NGPS__
			NxPs2::InitialiseEngine();
			Nx::CLoadScreen::sDisplay( "loadscrn", false, false );
			//Nx::CLoadScreen::sStartLoadingBar(duration);
			#endif

			
			// NOTE:  I moved SkateScript::Init() down here, as
			// we want to display the loading screen before
			// we load any other files (so it loads as fast as possible)																  
			// SO, NO SCRIPT FUNCTIONS MAY BE USED BEFORE THIS POINT

			Mem::PushMemProfile("SkateScript::Init()");
			SkateScript::Init();	 			

			Mem::PushMemProfile("Game Singletons");
			#ifdef __NOPT_ASSERT__
			Spt::SingletonPtr< Dbg::CScriptDebugger> script_debugger(true);
			#endif
			Mem::PopMemProfile();

#ifdef __PLAT_NGC__
extern uint8 * RES_gamecube;
			Spt::SingletonPtr< Script::CScriptCache>			script_cache( true );
			Script::ParseQB( "scripts\\engine\\gamecube.qb", RES_gamecube );

			if ( ( VIGetTvFormat() == VI_NTSC ) || ( VIGetTvFormat() == VI_MPAL ) )
			{
				NsDisplay::Check480P();
			}
			else
			{
				NsDisplay::Check60Hz();
			}
#endif		// __PLAT_NGC__
			SkateScript::Preload();
			Mem::PopMemProfile();
			
			Mem::PushMemProfile("Game Singletons");
			Obj::CPathMan::Instance(); // Force creation of CPathMan singleton

#ifndef __PLAT_NGC__
			Spt::SingletonPtr< Script::CScriptCache>			script_cache( true );
#endif		// __PLAT_NGC__
			Spt::SingletonPtr< Ass::CAssMan > 					ass_manager( true );
			Spt::SingletonPtr< Sfx::CSfxManager > 				sfx_manager( true );			
			Spt::SingletonPtr< Net::Manager >					net_manager( true );	
			Spt::SingletonPtr< Obj::CCompositeObjectManager >   composite_object_manager( true );
			Spt::SingletonPtr< Mdl::Skate >						skate_mod( true );
			Spt::SingletonPtr< Mdl::FrontEnd > 					front(true);
			Spt::SingletonPtr< GameNet::Manager > 				gamenet_man( true );
			Spt::SingletonPtr< Ed::CParkEditor>					grandpas_park_editor(true);

			Spt::SingletonPtr< Front::CScreenElementManager > 	screen_elem_manager(true);
#if __USE_REPLAYS__
			Spt::SingletonPtr< Replay::Manager > 				replay_manager( true );
#endif			
			Mem::PopMemProfile(/*"Game Singletons"*/);

			DEBUG_FLASH(0x007f7f7f);		// white

			Mem::PushMemProfile("Resgistering and starting modules");

			/***********************************************
			Setup main loop tasks section
			************************************************/

			// Note, some managers/modules add their tasks in their constructors
			// however, these three are hooked up in the main loop, for some reason
			// and have to provide a global function to get a pointer to
			// their appropiate tasks
		
//			mlp_manager->AddSystemTask ( tgt_manager->GetProcessBackGroundTasks() );
			mlp_manager->AddSystemTask ( mdl_manager->GetProcessModulesTask() );   // Note: this hooks up the code that starts the modules
			mlp_manager->AddSystemTask ( sio_manager->GetProcessDevicesTask() );
			mlp_manager->AddLogicTask  ( inp_manager->GetProcessHandlersTask() );
		
			/***********************************************
			Register modules section
			************************************************/
			
			/* 
				RJM: 
				
				The FrontEnd and Panel Managers must be initialized before the Skate Mod,
				so that the game can be launched from a startup script without problems
			
			*/
			
			mdl_manager->RegisterModule ( *script_cache );
			mdl_manager->RegisterModule ( *grandpas_park_editor );
			mdl_manager->RegisterModule ( *front );
			mdl_manager->RegisterModule ( *skate_mod );
			mdl_manager->RegisterModule ( *async_poll );
			#ifdef __NOPT_ASSERT__
			mdl_manager->RegisterModule ( *script_debugger );
			#endif
			
			/***********************************************
			Start modules section
			************************************************/
			
			mdl_manager->StartModule( *script_cache );
			mdl_manager->StartModule( *front );
			mdl_manager->StartModule( *skate_mod );      
			mdl_manager->StartModule ( *grandpas_park_editor );
			mdl_manager->StartModule ( *async_poll );
			
			Mem::PopMemProfile(/*"Resgistering and starting modules"*/);
		
						
			/***********************************************
			Main loop section
			************************************************/
			
#ifdef		__USE_PROFILER__
			Sys::Profiler::sSetUp(2);
#endif			
			
			Mem::PushMemProfile("sStartEngine");
			Nx::CEngine::sStartEngine();
			Mem::PopMemProfile(/*"sStartEngine"*/);

			Dbg_Message ( "Starting main loop" );
// Note to the adventurer:
// Where does control go now? .... you may well ask
// The above "modules" have simply been flagged as wanting to start, they have not
// actually started yet.
// The code that actually runs them is called ultimately from "service_system" in mainloop.cpp
// "service_system" iterates over the "system_task_list", and calls all the tasks
// in that list.  One of those tasks is the task returned by GetProcessModulesTask()
// which is added to the task list by the line:
// 			mlp_manager->AddSystemTask ( mdl_manager->GetProcessModulesTask() );
// above.
// The GetProcessModulesTask() returns (via some indirection), Mdl::Manager::process_modules
// and this function iterates over the modules registered with the module manager
// in part it checks if (current->command == Module::vSTART), and if so, then it calls
// 					current->v_start_cb();
// which in the case of the skate module is Mdl::Skate::v_start_cb
// which does some initialization of the skate module
// and in the course of that, calls 
//	Script::RunScript("default_system_startup");
//	Script::RunScript("StartUp");	<<< but not on the CD version
//
// The "Startup" script differs per person during development, but will typically look like:
//
//	script startup
//	    autolaunch level=load_sch game=career
//	endscript
//
// which calls a bunch of commands to ultimately load the level geometry,
// parse the triggers in the "node array" 
// and drop in the skater
// and start skating.
// phew

			Mem::PopMemProfile(/*Initilization*/);
			
			
			Mem::PushMemProfile("MainLoop");
			mlp_manager->MainLoop();
			Mem::PopMemProfile(/*"MainLoop"*/);
			
// The following code is theoretical, as we actually never shut down the whole game			
// On the PS2, we use the script command "LoadExecPS2", which calls "ScriptLoadExecPS2" in cfuncs.cpp
// which allows us to load another executable.  This would either be a demo or another game
// or it would be returning to the bootstrap loader.
			
			Dbg_Message ( "Ending main loop" );
			
#ifdef		__USE_PROFILER__
			Sys::Profiler::sCloseDown();
#endif			
			mdl_manager->StopAllModules();
			
			/***********************************************
			Unregister section
			************************************************/
			
			mdl_manager->UnregisterModule ( *skate_mod );
			mdl_manager->UnregisterModule ( *front );
			mdl_manager->UnregisterModule ( *grandpas_park_editor );
			mdl_manager->UnregisterModule ( *async_poll );
			#ifdef __NOPT_ASSERT__
			mdl_manager->UnregisterModule ( *script_debugger );
			#endif
			
			/***********************************************
			General closedown
			************************************************/
			
			mlp_manager->RemoveAllDisplayTasks();			
			mlp_manager->RemoveAllLogicTasks();
			mlp_manager->RemoveAllSystemTasks();

			Dbg_Message ( "End Application" );
		}
		Tmr::DeInit();
		Mem::Manager::sHandle().PopMemoryMarker(MAINLOOP_MEMMARKER);
		Mem::Manager::sHandle().BottomUpHeap()->PopContext();
	Mth::Matrix A,B,C;
	mat_test(A,B,C);				 
	}

				 
	return 0;
}

void nob(Mth::Matrix A);

void mat_test(const Mth::Matrix A,const Mth::Matrix B,Mth::Matrix C)
{

	Mth::Matrix D;
	D = A * B;
}

void nob(Mth::Matrix A)
{
	printf ("%f",A[X][X]);
}



/*****************************************************************************
**							                    							**
*****************************************************************************/






