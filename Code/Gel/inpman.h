/*****************************************************************************
**																			**
**					   	  Neversoft Entertainment							**
**																		   	**
**				   Copyright (C) 1999 - All Rights Reserved				   	**
**																			**
******************************************************************************
**																			**
**	Project:		GEL (Game Engine Library)								**
**																			**
**	Module:			Input  (Inp)											**
**																			**
**	File name:		gel/inpman.h											**
**																			**
**	Created: 		05/26/2000	-	spg										**
**																			**
*****************************************************************************/

#ifndef __GEL_INPMAN_H
#define __GEL_INPMAN_H

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/singleton.h>
#include <core/list.h>
#include <core/task.h>

#include <sys/sioman.h>
#include <sys/siodev.h>
 
/*****************************************************************************
**								   Defines									**
*****************************************************************************/


#define PAD_NONE 0
#define PAD_U 1
#define PAD_D 2
#define PAD_L 3
#define PAD_R 4
#define PAD_UL 5
#define PAD_UR 6
#define PAD_DL 7
#define PAD_DR 8
#define PAD_CIRCLE 9
#define PAD_SQUARE 10
#define PAD_X 11
#define PAD_TRIANGLE 12
#define PAD_L1 13
#define PAD_L2 14
#define PAD_L3 15
#define PAD_R1 16
#define PAD_R2 17
#define PAD_R3 18
#define PAD_BLACK 19
#define PAD_WHITE 20
#define PAD_Z 21
#define PAD_NUMBUTTONS 22

namespace Inp
{



static const int vANALOGUE_TOL      = 50;			// Mick, changed back from 32, as our controllers were wandering...
static const int vANALOGUE_CENTER   = 128;


extern uint32	gDebugButtons[];
extern uint32	gDebugBreaks[];
extern uint32	gDebugMakes[];


/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/

int GetButtonIndex(uint32 Checksum);
uint32 GetButtonChecksum( int whichButton );

class  Data  : public Spt::Class
{   
	

public:

        friend class Server;

						Data( void );

#define vMAX_ANALOG_VALUE   255

    enum DigitalButtonIndex
	{      
        vD_L2,
        vD_R2,
        vD_L1,
        vD_R1,
        vD_TRIANGLE,
        vD_CIRCLE,
        vD_X,
        vD_SQUARE,
        vD_SELECT,
        vD_L3,
        vD_R3,
        vD_START,
        vD_UP,
        vD_RIGHT,
        vD_DOWN,
		vD_LEFT,
		vD_BLACK,	// Only supported on XBox
		vD_WHITE,	// Only supported on XBox
		vD_Z,		// Only supported on NGC
        vMAX_DIGITAL_EVENTS
    };
       
    enum AnalogButtonIndex
    {
        vA_RIGHT_X,         // Right analog controller stick
		vA_RIGHT_Y,
		vA_LEFT_X,          // Left analog controller stick
		vA_LEFT_Y,
        vA_RIGHT,
        vA_LEFT,
        vA_UP,
        vA_DOWN,
        vA_TRIANGLE,
        vA_CIRCLE,
        vA_X,
        vA_SQUARE,
        vA_L1,
        vA_R1,
        vA_L2,
        vA_R2,
        vA_L3,
        vA_R3,
		// (Mick) Added the following to stop the analog values before they are clamped
		// these values only get clamped if BOTH X and Y are in the middle
		// the above values clamp X and Y independently
		// leading to dead zones when rotating the stick round in a circle
		// a significant loss in accuracy
        vA_RIGHT_X_UNCLAMPED,         // Right analog controller stick
		vA_RIGHT_Y_UNCLAMPED,
		vA_LEFT_X_UNCLAMPED,          // Left analog controller stick
		vA_LEFT_Y_UNCLAMPED,
		
		vA_BLACK,					// Only supported on XBox
		vA_WHITE,					// Only supported on XBox
		
		vA_Z,						// Only supported on XBox

//		vA_SELECT,		
					   
		vMAX_ANALOG_EVENTS
	};

	enum DigitalButtonMask
	{
        // These map directly to digital input in m_Buttons, m_Prev and m_New
        mD_L2           = nBit( vD_L2 ),
        mD_R2           = nBit( vD_R2 ),
        mD_L1           = nBit( vD_L1 ),
        mD_R1           = nBit( vD_R1 ),
        mD_TRIANGLE     = nBit( vD_TRIANGLE ),
		mD_CIRCLE       = nBit( vD_CIRCLE ),
        mD_X            = nBit( vD_X ),
        mD_SQUARE       = nBit( vD_SQUARE ),
        mD_SELECT       = nBit( vD_SELECT ),
        mD_L3           = nBit( vD_L3 ),
        mD_R3           = nBit( vD_R3 ),
        mD_START        = nBit( vD_START ),
        mD_UP           = nBit( vD_UP ),
        mD_RIGHT        = nBit( vD_RIGHT ),
        mD_DOWN         = nBit( vD_DOWN ),
        mD_LEFT         = nBit( vD_LEFT ),
		mD_BLACK		= nBit( vD_BLACK ),
		mD_WHITE		= nBit( vD_WHITE ),
		mD_Z			= nBit( vD_Z ),
        mD_ALL          = 0xffffffff
	};

    enum AnalogButtonMask
    {    
        // These map indirectly to analog input in m_Events
		mA_RIGHT_X      = nBit( vA_RIGHT_X ),         // Right analog controller stick
		mA_RIGHT_Y      = nBit( vA_RIGHT_Y ),
		mA_LEFT_X       = nBit( vA_LEFT_X ),          // Left analog controller stick
		mA_LEFT_Y       = nBit( vA_LEFT_Y ),
        mA_RIGHT        = nBit( vA_RIGHT ),
        mA_LEFT         = nBit( vA_LEFT ),
        mA_UP           = nBit( vA_UP ),
        mA_DOWN         = nBit( vA_DOWN ),
        mA_TRIANGLE     = nBit( vA_TRIANGLE ),
        mA_CIRCLE       = nBit( vA_CIRCLE ),
        mA_X            = nBit( vA_X ),
        mA_SQUARE       = nBit( vA_SQUARE ),
        mA_L1           = nBit( vA_L1 ),
        mA_R1           = nBit( vA_R1 ),
        mA_L2           = nBit( vA_L2 ),
        mA_R2           = nBit( vA_R2 ),
        mA_L3           = nBit( vA_L3 ),
        mA_R3           = nBit( vA_R3 ),
        mA_BLACK        = nBit( vA_BLACK ),
        mA_WHITE        = nBit( vA_WHITE ),
        mA_Z	        = nBit( vA_Z ),
//		mA_SELECT		= nBit( vA_SELECT ),
        mA_ALL          = 0xffffffff
	};
	

	void    MaskDigitalInput( int button_mask );
    void    MaskAnalogInput( int button_mask );
    void    ConvertDigitalToAnalog( void );
	void	OverrideAnalogPadWithStick( void );

	uint8   m_Event[vMAX_ANALOG_EVENTS];	// Analog info
	uint    m_Buttons;              // Current accessible button info (digital)   
    uint    m_Makes;                // Depressed this frame
    uint    m_Breaks;               // Released this frame

private:

	uint    m_new;                  // Edges (digital)
	uint    m_cur;              	// Current INTERNAL button info (digital)
	uint    m_prev;                 // last frame's trig
	
    void    handle_analog_tolerance( void );
};

class  RecordedData  : public Spt::Class
{
	
	
	friend class Server;

private:	
	sint8	m_valid;								// Was data valid on this frame?
	uint8   m_event[Data::vMAX_ANALOG_EVENTS];		// Analog info
	uint    m_cur;              					// Current INTERNAL button info (digital)     
	
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  BaseHandler : public  Tsk::BaseTask 
{
	

    friend class Manager;

public:
	Data*           m_Input;          // controller input  
    SIO::Device     *m_Device;        // the device from which this data was obtained
	int				m_Index;

protected:
                    BaseHandler ( int index, Tsk::BaseTask::Node::Priority pri )
					: Tsk::BaseTask( pri ), m_Index ( index ) 
					{
						Dbg_Printf( "Creating BaseHandler with Pri %d\n", pri );
					}
                    

    virtual         ~BaseHandler ( void ) {}
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

nTemplateSubClass( _T, Handler, BaseHandler )
{
	

public:

	typedef void	(Code)( const Handler< _T >& );

					Handler( int index, Code* const code, _T& data, 
                             Lst::Node< Tsk::BaseTask >::Priority pri = Lst::Node< Tsk::BaseTask >::vNORMAL_PRIORITY );
		
    virtual         ~Handler ( void );

	virtual void	vCall( void ) const;
	virtual void *	GetCode(void) const;
	_T&				GetData( void ) const;

private :

	Code* const		code;			// tasks entry point
	_T&				data;			// task defined data                    
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/                                                                   
                                                                   
//class  Server  : public Spt::Class // note - need compiler fix for delete[] bug!!!
class Server
{
//	 

	friend class Manager;

public:
				Server();
				
	void		RecordInput( RecordedData *data_buffer );
	void		PlaybackInput( RecordedData *recorded_input );
	
private:

    void        service_handlers( void );

	SIO::Device                 *m_device;
	Tsk::Stack	                m_handler_stack;
	Data                        m_data;
	
	RecordedData				*m_data_in;
	void						*m_data_in_end;
	RecordedData				*m_data_out;
	void						*m_data_out_end;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  Manager  : public Spt::Class
{
	                    

public :

	void                        AddHandler( BaseHandler &handler );
	void						ReassignHandler( BaseHandler &handler, int new_index );
	void                        AddPushHandler( BaseHandler &handler );
	Tsk::BaseTask&				GetProcessHandlersTask ( void ) const;
	
	void						RecordInput( int index, RecordedData *data_buffer, int byte_length );
	void						PlaybackInput( int index, RecordedData *recorded_input, int byte_length );
	void						PushInputLogicTasks( void );
	void						PopInputLogicTasks( void );
	bool						ActuatorEnabled( int i );
	
	void						DisableActuators( void );
	void						DisableActuator( int i );
	
	void						EnableActuators( void );
	void						EnableActuator( int i );
	void						ResetActuators( void );

	void						SetAnalogStickOverride( bool should_override_pad );
	bool						ShouldAnalogStickOverride( void );

private :
								~Manager ( void );
								Manager ( void );

	static Tsk::Task< Manager >::Code	process_handlers;
								
	Tsk::Task< Manager >*		m_process_handlers_task;
	Server                      m_server[SIO::vMAX_DEVICES];
	bool						m_override_pad_with_stick;
	
	DeclareSingletonClass( Manager );
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

inline	Tsk::BaseTask&	Manager::GetProcessHandlersTask ( void ) const
{
	
	
	Dbg_AssertType ( m_process_handlers_task, Tsk::BaseTask );

	return	*m_process_handlers_task;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void Manager::AddHandler ( BaseHandler &handler )
{
	

    Dbg_MsgAssert( handler.m_Index < SIO::vMAX_DEVICES,( "Invalid controller index" ));
	
    m_server[handler.m_Index].m_handler_stack.AddTask( handler );
    handler.m_Input = &m_server[handler.m_Index].m_data;
    handler.m_Device = m_server[handler.m_Index].m_device;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void Manager::ReassignHandler( BaseHandler &handler, int new_index )
{
	Dbg_MsgAssert( new_index < SIO::vMAX_DEVICES,( "Invalid controller index" ));

	handler.Remove();
	m_server[new_index].m_handler_stack.AddTask( handler );
    handler.m_Input = &m_server[new_index].m_data;
    handler.m_Device = m_server[new_index].m_device;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void Manager::AddPushHandler ( BaseHandler &handler )
{
	

	Dbg_MsgAssert( handler.m_Index < SIO::vMAX_DEVICES,( "Invalid controller index" ));
	
    m_server[handler.m_Index].m_handler_stack.AddPushTask( handler );
    handler.m_Input = &m_server[handler.m_Index].m_data;
    handler.m_Device = m_server[handler.m_Index].m_device;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline 
Handler< _T >::Handler( int index, Code* const _code, _T& _data, Tsk::BaseTask::Node::Priority pri )
: BaseHandler( index, pri ), code( _code ), data( _data ) 
{
    
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline 
Handler< _T >::~Handler( void )
{
    
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline 
void		Handler< _T >::vCall( void ) const
{
	
	
	Dbg_AssertPtr( code );
	code( *this );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline 
_T&		Handler< _T >::GetData( void ) const
{
	
	
	Dbg_AssertType( &data, _T );
	return data;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

template < class _T > inline 
void *		Handler< _T >::GetCode( void ) const
{
	

	return NULL;	
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

} // namespace Inp

#endif	// __GEL_INPMAN_H
