///////////////////////////////////////////////////////////////////////////////////////
//
// AsyncFilesys.cpp		GRJ 8 Oct 2002
//
// Asynchronous file system
//
///////////////////////////////////////////////////////////////////////////////////////

#include <sys/file/AsyncFilesys.h>
#include <gel/mainloop.h>

namespace File
{

///////////////////////////////////////////////////////////////////////////////////////
// Constants
//

const uint32	CAsyncFileHandle::MAX_FILE_SIZE = (uint32) ((uint64) (1 << 31) - 1);	// 1 Gig (Because we are using signed, we lose a bit)


/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CAsyncFileHandle::CAsyncFileHandle()
{
	init();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileHandle::init()
{
	// Init vars
	mp_callback = NULL;
	m_callback_arg0 = 0;
	m_callback_arg1 = 0;
	m_current_function = FUNC_IDLE;
	m_mem_destination = (EAsyncMemoryType) DEFAULT_MEMORY_TYPE;
	m_stream = false;
	m_blocking = true;			// Makes it async
	m_buffer_size = DEFAULT_BUFFER_SIZE;
	m_priority = DEFAULT_ASYNC_PRIORITY;

	m_file_size = -1;
	m_position = -1;
	m_busy_count = 0;
	m_last_result = 0;

	mp_pre_file = NULL;

	plat_init();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CAsyncFileHandle::~CAsyncFileHandle()
{
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

volatile bool		CAsyncFileHandle::IsBusy( void )
{
	// Don't think we need the plat_is_busy()
	return get_busy_count() > 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int					CAsyncFileHandle::WaitForIO( void )
{
	do
	{
		// Process any pending callbacks
		CAsyncFileLoader::s_execute_callback_list();
	} while (IsBusy());

	return m_last_result;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileHandle::SetPriority( int priority )
{
	//Dbg_MsgAssert(!IsBusy(), ("Can't set file parameters: asynchronous file handle already busy."));
	m_current_function = FUNC_IDLE;

	m_priority = priority;

	plat_set_priority(priority);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileHandle::SetStream( bool stream )
{
	//Dbg_MsgAssert(!IsBusy(), ("Can't set file parameters: asynchronous file handle already busy."));
	m_current_function = FUNC_IDLE;

	m_stream = stream;

	plat_set_stream(stream);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileHandle::SetDestination( EAsyncMemoryType destination )
{
	//Dbg_MsgAssert(!IsBusy(), ("Can't set file parameters: asynchronous file handle already busy."));
	m_current_function = FUNC_IDLE;

	m_mem_destination = destination;

	plat_set_destination(destination);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileHandle::SetBufferSize( size_t buffer_size )
{
	//Dbg_MsgAssert(!IsBusy(), ("Can't set file parameters: asynchronous file handle already busy."));
	m_current_function = FUNC_IDLE;

	m_buffer_size = buffer_size;

	plat_set_buffer_size(buffer_size);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileHandle::SetBlocking( bool block )
{
	//Dbg_MsgAssert(!IsBusy(), ("Can't set file parameters: asynchronous file handle already busy."));
	m_current_function = FUNC_IDLE;

	m_blocking = block;

	plat_set_blocking(block);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileHandle::SetCallback( AsyncCallback p_callback, unsigned int arg0, unsigned int arg1)
{
	mp_callback = p_callback;
	m_callback_arg0 = arg0;
	m_callback_arg1 = arg1;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
size_t				CAsyncFileHandle::GetFileSize( void )
{
	if (mp_pre_file)
	{
		Dbg_Assert(PreMgr::sPreEnabled());

        m_last_result = PreMgr::pre_get_file_size(mp_pre_file);

		Dbg_Assert(PreMgr::sPreExecuteSuccess());
		Dbg_MsgAssert(m_last_result, ("PRE file size is 0"));

		return m_last_result;
	}

	// Make sure we have a valid file size first
	while (m_file_size < 0)
		;

	return m_file_size;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

size_t				CAsyncFileHandle::Load(void *p_buffer)
{
	//Dbg_MsgAssert(!IsBusy(), ("Can't start new load: asynchronous file handle already busy."));

	m_current_function = FUNC_LOAD;

	if (mp_pre_file)
	{
		Dbg_Assert(PreMgr::sPreEnabled());

		int size = PreMgr::pre_get_file_size(mp_pre_file);
		PreMgr::pre_fseek(mp_pre_file, SEEK_SET, 0);
        m_last_result = PreMgr::pre_fread(p_buffer, 1, size, mp_pre_file);

		Dbg_Assert(PreMgr::sPreExecuteSuccess());

		inc_busy_count();
		io_callback(m_current_function, m_last_result, 0);			// Must call this when we are done
		return m_last_result;
	}

	return plat_load(p_buffer);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

size_t				CAsyncFileHandle::Read(void *p_buffer, size_t size, size_t count)
{
	//Dbg_MsgAssert(!IsBusy(), ("Can't start new read: asynchronous file handle already busy."));

	m_current_function = FUNC_READ;

	if (mp_pre_file)
	{
		Dbg_Assert(PreMgr::sPreEnabled());

		m_last_result = PreMgr::pre_fread(p_buffer, size, count, mp_pre_file);

		Dbg_Assert(PreMgr::sPreExecuteSuccess());

		inc_busy_count();
		io_callback(m_current_function, m_last_result, 0);			// Must call this when we are done
		return m_last_result;
	}

	return plat_read(p_buffer, size, count);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

size_t				CAsyncFileHandle::Write(void *p_buffer, size_t size, size_t count)
{
	//Dbg_MsgAssert(!IsBusy(), ("Can't start new write: asynchronous file handle already busy."));

	m_current_function = FUNC_WRITE;

	if (mp_pre_file)
	{
		Dbg_Assert(PreMgr::sPreEnabled());

		m_last_result = PreMgr::pre_fwrite(p_buffer, size, count, mp_pre_file);

		Dbg_Assert(PreMgr::sPreExecuteSuccess());

		inc_busy_count();
		io_callback(m_current_function, m_last_result, 0);			// Must call this when we are done
		return m_last_result;
	}

	return plat_write(p_buffer, size, count);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

int					CAsyncFileHandle::Seek(long offset, int origin)
{
	//Dbg_MsgAssert(!IsBusy(), ("Can't seek: asynchronous file handle already busy."));

	m_current_function = FUNC_SEEK;

	if (mp_pre_file)
	{
		Dbg_Assert(PreMgr::sPreEnabled());

		m_last_result = PreMgr::pre_fseek(mp_pre_file, offset, origin);

		Dbg_Assert(PreMgr::sPreExecuteSuccess());

		inc_busy_count();
		io_callback(m_current_function, m_last_result, 0);			// Must call this when we are done
		return m_last_result;
	}

	return plat_seek(offset, origin);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileHandle::io_callback(EAsyncFunctionType function, int result, uint32 data)
{
	m_last_result = result;
	CAsyncFileLoader::s_new_io_completion = true;

	// Call user function, if any
	if (mp_callback)
	{
		CAsyncFileLoader::s_add_callback(this, function, result);
	} else {
		post_io_callback();
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileHandle::post_io_callback()
{
	// Clean up
	Dbg_MsgAssert(get_busy_count() > 0, ("We will go into a neagtive busy with completion of function %d", m_current_function));
	if (dec_busy_count() == 0)
	{
		m_current_function = FUNC_IDLE;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CAsyncFileHandle::open(const char *filename, bool blocking, int priority)
{
	Dbg_MsgAssert(!IsBusy(), ("Can't open file %s: asynchronous file handle already busy.", filename));
	Dbg_MsgAssert(!mp_pre_file, ("Can't open file %s: already open as a PRE file.", filename));

	m_priority = priority;
	m_blocking = blocking;

	m_current_function = FUNC_OPEN;

    if (PreMgr::sPreEnabled())
    {
        mp_pre_file = PreMgr::pre_fopen(filename, "rb", true);
        if (mp_pre_file)
		{
			Dbg_Assert(PreMgr::sPreExecuteSuccess());

			m_blocking = true;		// Since PRE files are in memory, they will finish "instantly"
			m_last_result = true;

			inc_busy_count();
			io_callback(m_current_function, m_last_result, 0);			// Must call this when we are done
			return m_last_result;
		}
    }

	return plat_open(filename);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CAsyncFileHandle::close()
{
	m_current_function = FUNC_CLOSE;

	if (mp_pre_file)
	{
		Dbg_Assert(PreMgr::sPreEnabled());

        m_last_result = PreMgr::pre_fclose(mp_pre_file, true);

		Dbg_Assert(PreMgr::sPreExecuteSuccess());

		inc_busy_count();
		io_callback(m_current_function, m_last_result, 0);			// Must call this when we are done
		return m_last_result;
	}

	return plat_close();
}

/******************************************************************/
/*                                                                */
/* plat stubs													  */
/*                                                                */
/******************************************************************/

void				CAsyncFileHandle::plat_init()
{
	printf ("STUB: CAsyncFileHandle::Init\n");
}

bool				CAsyncFileHandle::plat_open(const char *filename)
{
	printf ("STUB: CAsyncFileHandle::Open\n");

	return false;
}

bool				CAsyncFileHandle::plat_close()
{
	printf ("STUB: CAsyncFileHandle::Close\n");

	return false;
}

volatile bool		CAsyncFileHandle::plat_is_done()
{
	printf ("STUB: CAsyncFileHandle::IsDone\n");

	return false;
}

volatile bool		CAsyncFileHandle::plat_is_busy()
{
	printf ("STUB: CAsyncFileHandle::IsBusy\n");

	return false;
}

bool				CAsyncFileHandle::plat_is_eof() const
{
	printf ("STUB: CAsyncFileHandle::IsEOF\n");

	return false;
}

void				CAsyncFileHandle::plat_set_priority( int priority )
{
	printf ("STUB: CAsyncFileHandle::SetPriority\n");
}

void				CAsyncFileHandle::plat_set_stream( bool stream )
{
	printf ("STUB: CAsyncFileHandle::SetStream\n");
}

void				CAsyncFileHandle::plat_set_destination( EAsyncMemoryType destination )
{
	printf ("STUB: CAsyncFileHandle::SetDestination\n");
}

void				CAsyncFileHandle::plat_set_buffer_size( size_t buffer_size )
{
	printf ("STUB: CAsyncFileHandle::SetBufferSize\n");
}

void				CAsyncFileHandle::plat_set_blocking( bool block )
{
	printf ("STUB: CAsyncFileHandle::SetBlocking\n");
}

size_t				CAsyncFileHandle::plat_load(void *p_buffer)
{
	printf ("STUB: CAsyncFileHandle::Load\n");

	return 0;
}

size_t				CAsyncFileHandle::plat_read(void *p_buffer, size_t size, size_t count)
{
	printf ("STUB: CAsyncFileHandle::Read\n");

	return 0;
}

size_t				CAsyncFileHandle::plat_write(void *p_buffer, size_t size, size_t count)
{
	printf ("STUB: CAsyncFileHandle::Write\n");

	return 0;
}

char *				CAsyncFileHandle::plat_get_s(char *p_buffer, int maxlen)
{
	printf ("STUB: CAsyncFileHandle::GetS\n");

	return NULL;
}

int					CAsyncFileHandle::plat_seek(long offset, int origin)
{
	printf ("STUB: CAsyncFileHandle::Seek\n");

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////

CAsyncFileHandle	*	CAsyncFileLoader::s_file_handles[MAX_FILE_HANDLES];
int						CAsyncFileLoader::s_free_handle_index = 0;

volatile int			CAsyncFileLoader::s_manager_busy_count = 0;
volatile bool			CAsyncFileLoader::s_new_io_completion = false;

volatile CAsyncFileLoader::SCallback	CAsyncFileLoader::s_callback_list[2][MAX_PENDING_CALLBACKS];
volatile int							CAsyncFileLoader::s_num_callbacks[2] = { 0, 0 };
int										CAsyncFileLoader::s_cur_callback_list_index = 0;

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileLoader::sInit()
{
	s_free_handle_index = 0;
	s_plat_init();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileLoader::sCleanup()
{
	s_plat_cleanup();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CAsyncFileLoader::sAsyncSupported()
{
	return s_plat_async_supported();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CAsyncFileLoader::sExist(const char *filename)
{
	return s_plat_exist(filename);
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CAsyncFileHandle *	CAsyncFileLoader::sOpen(const char *filename, bool blocking, int priority)
{
	CAsyncFileHandle *p_file_handle = s_get_file_handle();

	if (p_file_handle)
	{
		if (!p_file_handle->open(filename, blocking, priority))
		{
			//Dbg_MsgAssert(0, ("Error opening Async file %s", filename));
			s_free_file_handle(p_file_handle);
			return NULL;
		}

		return p_file_handle;
	}
	else
	{
		Dbg_MsgAssert(0, ("Out of Async handles"));
		return NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CAsyncFileLoader::sClose(CAsyncFileHandle *p_file_handle)
{

	bool result = p_file_handle->close();

	bool free_result = s_free_file_handle(p_file_handle);
	if ( !free_result )
	{
		Dbg_MsgAssert(0, ("sClose(): Can't find async handle in CAsyncFileLoader"));
	}

	return result;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void				CAsyncFileLoader::sWaitForIOEvent(bool all_io_events)
{
	bool done = false;

	while (!done)
	{
		printf("CAsyncFileLoader waiting for io completion: busy count %d completion %d\n", s_manager_busy_count, s_new_io_completion);

		// Wait for an event
		while (!s_new_io_completion)
			;

		printf("CAsyncFileLoader got completion: busy count %d completion %d\n", s_manager_busy_count, s_new_io_completion);

		// execute callbacks
		s_execute_callback_list();

		done = true;	// assume we are done for now
		if (all_io_events)
		{
			for (int i = 0; i < MAX_FILE_HANDLES; i++)
			{
				if (s_file_handles[i]->IsBusy())
				{
					printf("CAsyncFileLoader still needs to wait for file handle %d\n", i);
					Dbg_MsgAssert(s_manager_busy_count, ("CAsyncFileLoader busy count is 0 while file handle %d is busy", i));
					done = false;
					break;
				}
			}
		}
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CAsyncFileLoader::sAsyncInUse()
{
#if 0
	for (int i = 0; i < MAX_FILE_HANDLES; i++)
	{
		if (s_file_handles[i]->IsBusy())
		{
			return true;
		}
	}

	return false;
#endif

	Dbg_MsgAssert(s_manager_busy_count >= 0, ("CAsyncFileLoader busy count is negative: %d", s_manager_busy_count));
	return s_manager_busy_count > 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

CAsyncFileHandle *	CAsyncFileLoader::s_get_file_handle()
{
	if (s_free_handle_index < MAX_FILE_HANDLES)
	{
		// Find a non-busy handle and exchange with busy, if necessary
		if (s_file_handles[s_free_handle_index]->IsBusy())
		{
			int i;
			for (i = s_free_handle_index + 1; i < MAX_FILE_HANDLES; i++)
			{
				if (!s_file_handles[i]->IsBusy())
				{
					CAsyncFileHandle *p_temp = s_file_handles[i];
					s_file_handles[i] = s_file_handles[s_free_handle_index];
					s_file_handles[s_free_handle_index] = p_temp;
					break;
				}
			}
			
			// If we are full, wait for this one
			if (i == MAX_FILE_HANDLES)
			{
				Dbg_Message("CAsyncFileLoader::sOpen(): waiting for old handle to finish up");
				s_file_handles[s_free_handle_index]->WaitForIO();
				Dbg_Message("CAsyncFileLoader::sOpen(): done waiting.");
			}
		}

		s_file_handles[s_free_handle_index]->init();		// Clear out any old stuff first

		return s_file_handles[s_free_handle_index++];
	}
	else
	{
		return NULL;
	}
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

bool				CAsyncFileLoader::s_free_file_handle(CAsyncFileHandle *p_file_handle)
{
	Dbg_Assert(s_free_handle_index);

	for (int i = 0; i < MAX_FILE_HANDLES; i++)
	{
		if (p_file_handle == s_file_handles[i])
		{
			// Found it, exchange it with last open handle
			s_file_handles[i] = s_file_handles[--s_free_handle_index];
			s_file_handles[s_free_handle_index] = p_file_handle;
			return true;
		}
	}

	return false;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileLoader::s_add_callback(CAsyncFileHandle *p_file, EAsyncFunctionType function, int result)
{
	// This will be called from an interrupt
	//scePrintf("Adding callback for handle %x\n", p_file);
	Dbg_MsgAssert(s_num_callbacks[s_cur_callback_list_index] < MAX_PENDING_CALLBACKS, ("add_callback(): list is full"));

	volatile SCallback * p_callback_entry =  &s_callback_list[s_cur_callback_list_index][s_num_callbacks[s_cur_callback_list_index]++];

	p_callback_entry->mp_file_handle	= p_file;
	p_callback_entry->m_function		= function;
	p_callback_entry->m_result			= result;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/
void				CAsyncFileLoader::s_update()
{
	s_plat_update();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void				CAsyncFileLoader::s_execute_callback_list()
{
	s_plat_swap_callback_list();

	int list_index = s_cur_callback_list_index ^ 1;	// Look at old list
	int num_callbacks = s_num_callbacks[list_index];

	for (int i = 0; i < num_callbacks; i++)
	{
		volatile SCallback * p_callback_entry =  &s_callback_list[list_index][i];

		CAsyncFileHandle *p_file = p_callback_entry->mp_file_handle;
		Dbg_Assert(p_file->mp_callback);

		//Dbg_Message("Executing callback for handle %x", p_file);

		// Call the callback and the post callback
		(*p_file->mp_callback)(p_file, p_callback_entry->m_function, p_callback_entry->m_result,
							   p_file->m_callback_arg0, p_file->m_callback_arg1);
		p_file->post_io_callback();
	}

	s_num_callbacks[list_index] = 0;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

DefineSingletonClass( CAsyncFilePoll, "Async File System Poll module" );

CAsyncFilePoll::CAsyncFilePoll()
{
	mp_logic_task = new Tsk::Task< CAsyncFilePoll > ( CAsyncFilePoll::s_logic_code, *this );
}

CAsyncFilePoll::~CAsyncFilePoll()
{
	delete mp_logic_task;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAsyncFilePoll::v_start_cb ( void )
{
	Mlp::Manager * mlp_manager = Mlp::Manager::Instance();
	mlp_manager->AddLogicTask( *mp_logic_task );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAsyncFilePoll::v_stop_cb ( void )
{
	mp_logic_task->Remove();
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

void CAsyncFilePoll::s_logic_code ( const Tsk::Task< CAsyncFilePoll >& task )
{
	Dbg_AssertType ( &task, Tsk::Task< CAsyncFilePoll > );

	CAsyncFileLoader::s_update();
	CAsyncFileLoader::s_execute_callback_list();
}

}



