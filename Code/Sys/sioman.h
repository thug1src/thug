/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		SYS Library												**
**																			**
**	Module:			SIO (SIO_)   											**
**																			**
**	Created:		05/26/2000	-	spg										**
**																			**
**	File name:		sys/sioman.h											**
**																			**
*****************************************************************************/

#ifndef __SYS_SIOMAN_H
#define __SYS_SIOMAN_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#include <core/singleton.h>
#include <core/list.h>
#include <core/task.h>

#include <sys/siodev.h>

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

namespace SIO
{

    

	enum
	{
		
#		if defined( __PLAT_XBOX__ )
		vMAX_PORT = 4,
#		elif defined( __PLAT_NGC__ )
		vMAX_PORT = 4,
#		else
		vMAX_PORT = 2,
#		endif
		vMAX_SLOT = 1,
		vMAX_DEVICES = ( vMAX_PORT * vMAX_SLOT )
	};   

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

class  Manager  : public Spt::Class
{
	

	typedef Lst::Head< Device >			DeviceList;         

public :
	
	Tsk::BaseTask&				GetProcessDevicesTask ( void );
    Device*                     GetDeviceByIndex( int index );
    Device*                     GetDevice( int port, int slot );
	void						Pause();
	void						UnPause();
	
#	if defined( __PLAT_XBOX__ )
	void						ProcessDevices();
#	endif

private : 
	virtual						~Manager();
								Manager();                                              
    
    Device*					    create_device( int index, int port, int slot );
        
    static	Tsk::Task< DeviceList >::Code	process_devices;    
    
    DeviceList					m_devices;
    
    Tsk::Task< DeviceList >*	m_process_devices_task; 
    
	DeclareSingletonClass( Manager );
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  Search : public  Lst::Search< Device >
{
	

public :
					Search (void) {} 
					~Search (void) {} 
private :
	
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

int LoadIRX(const char *pName, int num_args = 0, char* args = NULL, bool assert_on_fail = true);

/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

inline	Tsk::BaseTask&	Manager::GetProcessDevicesTask ( void )
{
	
	
	Dbg_AssertType ( m_process_devices_task, Tsk::BaseTask );

	return *m_process_devices_task;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace SIO

#endif	// __SYS_SIOMAN__H

