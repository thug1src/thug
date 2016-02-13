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
**	Module:			NET  (Net)												**
**																			**
**	File name:		gel/net.h												**
**																			**
**	Created: 		01/29/01	-	spg										**
**																			**
*****************************************************************************/

#ifndef __NET_H__
#define __NET_H__

/*****************************************************************************
**							  	  Includes									**
*****************************************************************************/

#ifndef __CORE_DEFINES_H
#include <core/defines.h>
#endif
#include <core/list.h>
#include <core/task.h>
#include <core/singleton.h>
#include <core/thread.h>
#include <core/support/ref.h>

#include <sys/timer.h>

#if __PLAT_NGPS__
#include <sifrpc.h>        /* For sceSifInitRpc()                    */
#include <sifdev.h>        /* For sceSifLoadModule()                 */

#include "sntypes.h"       /* SN Systems types                       */
#include "sneeutil.h"      /* SN Systems PS2 EE Utilites (General)   */
#include "snsocket.h"      /* SN Systems socket API header file      */
#include "sntcutil.h"      /* SN Systems PS2 EE Utilites (TCP/IP)    */
#endif	// __PLAT_NGPS__

#ifdef __PLAT_WN32__
#include <winsock2.h>
#endif // __PLAT_WN32__

#ifdef __PLAT_XBOX__
#include <xtl.h>
#include <winsockx.h>
#endif // __PLAT_XBOX__

#ifdef __PLAT_NGC__
//// GameCube only.
//#include <sys/ngc/p_hw.h>
//#include "sys/ngc/p_dl.h"
//#include "sys/ngc/p_scene.h"
//#include "sys/ngc/p_slerp.h"
//#include "sys/ngc/p_vector.h"
//#include "sys/ngc/p_matrix.h"
//#include "sys/ngc/p_texman.h"
#include <dolphin/socket.h>
#define NsWorldSector	NsDL
#define gethostbyname(name) SOGetHostByName(name)
#define gethostname(name,namelen) 0
#define inet_addr(cp) 0
#define inet_aton(cp,addr) SOInetAtoN(cp, addr)
#define inet_ntoa(in) SOInetNtoA(in)
#define in_addr SOInAddr
#define sockaddr_in SOSockAddrIn
#define sockaddr SOSockAddr
#define AF_INET         SO_PF_INET              /* internetwork: UDP, TCP, etc. */
#define PF_INET         AF_INET
#define htonl(l)        SOHtoNl(l)
#define ntohl(l)        SONtoHl(l)
#define htons(s)        SOHtoNs(s)
#define ntohs(s)        SONtoHs(s)
#define sin_family family
#define sin_port port
#define sin_addr addr
#define sin_zero zero
#define s_addr addr
#define INADDR_ANY	SO_INADDR_ANY
#define IPPROTO_UDP 0

#define accept(s,addr,addrlen) SOAccept(s,addr)
#define bind(s,addr,addrlen) SOBind(s,addr)
#define closesocket(s) SOClose(s)
#define connect(s,addr,addrlen) SOConnect(s,addr)
#define getpeername(s,addr,addrlen) SOGetPeerName(s,addr)
#define getsockname(s,addr,addrlen) SOGetSockName(s,addr)
#define getsockopt(s,level,optname,optval,optlen) SOGetSockOpt(s,level,optname,optval)
#define listen(s,backlog) SOListen(s,backlog)
#define recv(s,buf,len,flags) SORecv(s,buf,len,flags)
#define recvfrom(s,buf,len,flags,from,fromlen) SORecvFrom(s,buf,len,flags,from)
#define recvmsg(s,msg,flags) 0
#define select(nfds,readfs,writefs,exceptfs,timeout) 0
#define send(s,buf,len,flags) SOSend(s,buf,len,flags)
#define sendmsg(s,msg,flags) 0
#define sendto(s,buf,len,flags,to,tolen) SOSendTo(s,buf,len,flags,to)
#define setsockopt(s,level,optname,optval,optlen) SOSetSockOpt(s,level,optname,optval)
#define shutdown(s,how) SOShutdown(s,how)
#define sockAPIinit(maxthreads) 0
#define sockAPIreinit() 0
#define sockAPIregthr() 0
#define sockAPIderegthr() 0
#define socket(af,type,protocol) SOSocket(af,type,protocol)


#define ENOBUFS         1
#define ETIMEDOUT       2
#define EISCONN         3
#define EOPNOTSUPP      4
#define ECONNABORTED    5
#define EWOULDBLOCK     6
#define ECONNREFUSED    7
#define ECONNRESET      8
#define ENOTCONN        9
#define EALREADY        10
#define EINVAL          11
#define EMSGSIZE        12
#define EPIPE           13
#define EDESTADDRREQ    14
#define ESHUTDOWN       15
#define ENOPROTOOPT     16
#define EHAVEOOB        17
#define ENOMEM          18
#define EADDRNOTAVAIL   19
#define EADDRINUSE      20
#define EAFNOSUPPORT    21
#define EINPROGRESS     22
#define ELOWER          23
#define EBADF           24

#define SOCK_STREAM     1               /* stream socket */
#define SOCK_DGRAM      2               /* datagram socket */

#define SO_DEBUG        0x0001          /* turn on debugging info recording */
#define SO_ACCEPTCONN   0x0002          /* socket has had listen() */
#define SO_REUSEADDR    0x0004          /* allow local address reuse */
#define SO_KEEPALIVE    0x0008          /* keep connections alive */
#define SO_DONTROUTE    0x0010          /* just use interface addresses */
#define SO_BROADCAST    0x0020          /* permit sending of broadcast msgs */
#define SO_USELOOPBACK  0x0040          /* bypass hardware when possible */
#define SO_LINGER       0x0080          /* linger on close if data present */
#define SO_OOBINLINE    0x0100          /* leave received OOB data in line */

/* Additional options not kept in so_options */

#define SO_SNDBUF       0x1001          /* send buffer size */
#define SO_RCVBUF       0x1002          /* receive buffer size */
#define SO_SNDLOWAT     0x1003          /* send low-water mark */
#define SO_RCVLOWAT     0x1004          /* receive low-water mark */
#define SO_SNDTIMEO     0x1005          /* send timeout */
#define SO_RCVTIMEO     0x1006          /* receive timeout */
#define SO_ERROR        0x1007          /* get error status and clear */
#define SO_TYPE         0x1008          /* get socket type */

#define SO_HOPCNT       0x1009          /* Get hop count to destination */
#define SO_MAXMSG       0x1010          /* Get TCP_MSS (max segment size) */

/* Option extensions */

#define SO_RXDATA       0x1011          /* Get count of received bytes */
#define SO_MYADDR       0x1012          /* Get local IP address */
#define SO_NBIO         0x1013          /* Set socket non-blocking */
#define SO_BIO          0x1014          /* Set socket blocking */
#define SO_NONBLOCK     0x1015          /* Set/get blocking state */

#define MSG_OOB         0x1             /* process out-of-band data */
#define MSG_PEEK        0x2             /* peek at incoming message */
#define MSG_DONTROUTE   0x4             /* send without using routing tables */
#define MSG_DONTWAIT    0x20            /* this message should be nonblocking */

#define SD_RECEIVE  0
#define SD_SEND     1
#define SD_BOTH     2
#endif // __PLAT_NGC__

/*****************************************************************************
**								   Defines									**
*****************************************************************************/

#ifdef __PLAT_NGPS__
#define vBASE_SOCKET_THREAD_ID	0x110

#define vSOCKET_THREAD_PRIORITY	10
#define vMAIN_THREAD_PRIORITY	11

#define vSOCKET_THREAD_STACK_SIZE	( 8 * 1024 )
#define vMODEM_THREAD_STACK_SIZE	( 12 * 1024 )
#endif

#ifndef SOCKET_ERROR
#	define SOCKET_ERROR -1
#endif

#ifndef INVALID_SOCKET
#	define INVALID_SOCKET -1
#endif

#if( defined( __PLAT_NGPS__ ) || defined( __PLAT_NGC__ ))
typedef int	SOCKET;	// Xbox type SOCKET is defined in winsockx.h.
#endif

//#define NET_DEBUG_MESSAGES

namespace Net
{



enum
{
	MSG_ID_PING_TEST			= 1,		// = 1 : Server->Client
	MSG_ID_PING_RESPONSE,					// = 2 : Client->Server
	MSG_ID_CONNECTION_REQ,					// = 3 : Client->Server
	MSG_ID_CONNECTION_ACCEPTED,				// = 4 : Server->Client
	MSG_ID_CONNECTION_REFUSED,				// = 5 : Server->Client
	MSG_ID_CONNECTION_TERMINATED,			// = 6 : Server->Client
	MSG_ID_SEQUENCED,						// = 7 : Bi-directional sequenced identifier
	MSG_ID_ACK,								// = 8 : Bi-directional acknowledgement
	MSG_ID_TIMESTAMP,						// = 9 : Server->Client
	MSG_ID_ALIAS,							// = 10 : Client->Server New alias request
	MSG_ID_DISCONN_REQ,						// = 11 : Client->Server Request to disconnect
	MSG_ID_DISCONN_ACCEPTED,				// = 12 : Server->Client Go ahead and quit
	MSG_ID_KEEPALIVE,						// = 13 : Bi-directional general keepalive message		
	MSG_ID_STREAM_START,					// = 14 : Bi-directional general stream start message
	MSG_ID_STREAM_DATA,						// = 15 : Bi-directional general stream data message
	MSG_ID_SERVER_RESPONSE,					// = 16 : Server->Client
	MSG_ID_FIND_SERVER,						// = 17 : Client->Server Broadcast

	MSG_ID_USER					= 32,		// Game-Specific messages start here
};

enum
{
	GROUP_ID_DEFAULT,
	GROUP_ID_LATENCY_TESTS,
	GROUP_ID_STREAMS,
};

#define MAX_NET_MSG_ID	255
#define MAX_MSG_IDS		256
#define MAX_SEQ_GROUPS	256
#define MAX_LEN_APP_NAME 64
#define MAX_STREAM_CHUNK_LENGTH	256
#define MAX_STREAM_LENGTH (Net::Conn::vREAD_BUFFER_LENGTH)
#define MAX_UDP_PACKET_SIZE 1300
#define vUDP_PACKET_OVERHEAD 28 // 20 for IPv4 header, 8 for UDP Header

#define NET_NUM_LATENCY_TESTS	10

enum
{
	HANDLE_ID_EXCLUDE_BROADCAST	= 0x80,	// Or this with a handle ID and it will send to all
										// handles except that id
	HANDLE_ID_BROADCAST 		= 255,
};

enum
{
	vNO_ALIAS  		= 255,
	vNUM_ALIASES	= 255,
	vALIAS_DURATION = 2000	// number of ms before an alias expires (2 seconds)
};

enum
{
	LOWEST_PRIORITY = 0,
	NORMAL_PRIORITY	= 128,
	HIGHEST_PRIORITY = 255
};

// MSG_HANDLER_FLAGS
enum
{
	mHANDLE_FOREIGN 		= 0x0001,
	mHANDLE_LATE			= 0x0002,
	mHANDLE_CRC_MISMATCH	= 0x0004,
};

enum
{
	HANDLER_ERROR,
	HANDLER_CONTINUE,
	HANDLER_HALT,
	HANDLER_MSG_DONE,
	HANDLER_MSG_DESTROYED,
};
	
typedef enum
{
	QUEUE_DEFAULT,
	QUEUE_IMPORTANT,
	QUEUE_SEQUENCED

} QUEUE_TYPE;

enum ConnType
{
	vCONN_TYPE_NONE,
	vCONN_TYPE_MODEM,
	vCONN_TYPE_ETHERNET,
	vCONN_TYPE_PPPOE
};

enum DeviceType
{
	vDEV_TYPE_NONE,
	vDEV_TYPE_USB_ETHERNET,
	vDEV_TYPE_USB_MODEM,
	vDEV_TYPE_PC_ETHERNET,
	vDEV_TYPE_SONY_MODEM,
};

enum
{
	vMODEM_CONNECT_TIMEOUT		= 60,	// seconds
	vMODEM_DISCONNECT_TIMEOUT	= 10,	// seconds
	vMODEM_RESET_TIMEOUT		= 10,	// seconds
};

enum
{
	vRES_SUCCESS,
	vRES_ERROR_GENERAL,
	vRES_ERROR_UNKNOWN_DEVICE,
	vRES_ERROR_INVALID_IRX,
	vRES_ERROR_DEVICE_NOT_CONNECTED,
	vRES_ERROR_DEVICE_NOT_HOT,
	vRES_ERROR_DHCP,
	vRES_ERROR_DEVICE_CHANGED,
};

enum
{
	vMODEM_STATE_DIALING,
	vMODEM_STATE_CONNECTED,
	vMODEM_STATE_LOGGED_IN,
	vMODEM_STATE_DISCONNECTING,
	vMODEM_STATE_HANGING_UP,
	vMODEM_STATE_DISCONNECTED,
	vMODEM_STATE_READY_TO_TRANSMIT,
	vMODEM_STATE_ERROR,
};

enum
{
	mMSG_SIZE_UNKNOWN = 0x01
};

class App;
class Server;
class Client;
class Conn;
class Manager;

/*****************************************************************************
**							Class Definitions								**
*****************************************************************************/
// STANDARD NET MESSAGES
class MsgMax
{
public:
	char	m_Data[4096];
};

class MsgTimestamp
{
public:
	unsigned int m_Timestamp;
};

class MsgPacketStamp
{
public:
	//unsigned char  m_Handle;
	unsigned short m_Packetstamp;
};

typedef MsgPacketStamp	MsgAck;

class MsgAlias
{
public:
	unsigned char	m_Alias;
	int				m_ObjId;
	int				m_Expiration;	// Frame of expiration
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class MsgStreamStart
{
public:
	char	m_StreamDesc[32];
	int		m_Size;
	unsigned int	m_StreamId;
	uint32	m_Checksum;
	unsigned char	m_MsgId;
	char	m_Data[ MAX_STREAM_CHUNK_LENGTH ];
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class MsgStreamData
{
public:
	unsigned int	m_StreamId;
	char			m_Data[ MAX_STREAM_CHUNK_LENGTH ];
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class StreamDesc
{
public:
	StreamDesc( void );
	~StreamDesc( void );
	int			m_Size;
	unsigned int	m_StreamId;
	unsigned char		m_MsgId;
	char		m_StreamDesc[32];
	char*		m_Data;
	char* 		m_DataPtr;
	unsigned char	m_GroupId;
	unsigned int	m_SequenceId;
	uint32		m_Checksum;
	bool		m_SendInPlace;
};

class StreamLink : public Lst::Node< StreamLink >
{
public:
	StreamLink( StreamDesc* desc );

	StreamDesc*	m_Desc;
};

// NET MESSAGE LINKS
class QueuedMsg
{
public:
	QueuedMsg( unsigned char msg_id, unsigned short msg_len, void* data );
	~QueuedMsg( void );

	unsigned char	m_MsgId;
	unsigned short	m_MsgLength;
	char*			m_Data;
	Spt::Ref		m_Ref;
};

class QueuedMsgSeq
{
public:
	QueuedMsgSeq( unsigned char msg_id, unsigned short msg_len, void* data, unsigned char group_id );
	~QueuedMsgSeq( void );

	unsigned char	m_MsgId;
	unsigned short	m_MsgLength;
	char*			m_Data;
	char			m_GroupId;
	
	Spt::Ref	m_Ref;
};

class MsgLink : public Lst::Node< MsgLink >
{
public:
	MsgLink( QueuedMsg* msg );

	QueuedMsg*		m_QMsg;
	unsigned int	m_SendTime;
};

class MsgImpLink : public Lst::Node< MsgImpLink >
{	
public:
	MsgImpLink( QueuedMsg* msg );

public:
	unsigned int 	m_Timestamp;
	QueuedMsg*		m_QMsg;
	unsigned int	m_Packetstamp;
	unsigned char	m_NumResends;
	unsigned int	m_SendTime;
};

class MsgSeqLink : public Lst::Node< MsgSeqLink >
{
public:
	MsgSeqLink( QueuedMsgSeq* msg );

	unsigned char	m_NumResends;
	unsigned char	m_GroupId;
	char			m_StreamMessage;	// boolean
	unsigned int	m_Packetstamp;
	unsigned int 	m_Timestamp;
	unsigned int	m_SequenceId;   
	unsigned int	m_SendTime;

	QueuedMsgSeq*	m_QMsg;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class AliasEntry
{
public:
	int	m_Id;			// full id of aliased object
	int m_Expiration;	// frame of expiration
};

class MsgHandlerContext
{
public:
	
	char*			m_Msg;
	unsigned char	m_MsgId;
	unsigned long   m_MsgLength;
	App*			m_App;
	Conn*			m_Conn;
	int				m_PacketFlags;
	void*			m_Data;
};

typedef int	(MsgHandlerCode)( MsgHandlerContext *context );

class MsgHandler : public Lst::Node< MsgHandler >
{
	friend class Dispatcher;
	friend class Server;
	friend class Client;

public:
	MsgHandler( MsgHandlerCode *code, int flags = 0, 
						void *data = NULL, int pri = NORMAL_PRIORITY );

private:
	MsgHandlerCode	*m_code;
	void			*m_data;
	int				m_flags;
	
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class LatencyTest
{
public:

	LatencyTest ();

	void	InputLatencyValue( int latency );

	int				m_AveLatency;
	unsigned int	m_TimeTests[NET_NUM_LATENCY_TESTS];
	unsigned int	m_CurrentTest;
	unsigned int	m_SendTime;
	unsigned int	m_ReceiveTime;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  PacketInfo  : public Spt::Class
{	
public:
	int			GetNumBytes( void );
	int			GetTime( void );

	void		SetNumBytes( int size );
	void		SetTime( int time );

private:
	int			m_num_bytes;
	int			m_time;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  Metrics  : public Spt::Class
{
public:
	enum
	{
		vNUM_BUFFERED_PACKETS = 256
	};

				Metrics( void );

	void		CalculateBytesPerSec( int cur_time );
	int			GetBytesPerSec( void );
	
	int			GetTotalBytes( void );
	int			GetTotalMessageData( int msg_id );
	int			GetTotalNumMessagesOfId( int msg_id );
		
	void		AddPacket( int size, int time );
	void		AddMessage( int msg_id, int size );
    
private:

	PacketInfo	m_packets[ vNUM_BUFFERED_PACKETS ];
	int			m_num_packets;
		
	int			m_num_messages[ MAX_MSG_IDS ];		// total # messages per msg id
	int			m_size_messages[ MAX_MSG_IDS ];		// total # bytes per msg id

	int			m_total_bytes;
	int			m_bytes_per_sec;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

// NET CONNECTION

class Conn
{
	friend class Client;
	friend class Server;
	friend class App;
	friend class Dispatcher;

private:
	
	Lst::Head< MsgLink >	m_normal_msg_list;
	Lst::Head< MsgImpLink >	m_important_msg_list;
	Lst::Head< MsgSeqLink >	m_sequenced_msg_list;

	friend	MsgHandlerCode	handle_ack;	
	friend	MsgHandlerCode	handle_latency_response;
	friend	MsgHandlerCode	handle_latency_test;

	int				m_handle;
	int				m_ip;
	unsigned short	m_port;
	
	char*			m_write_buffer;
	char*			m_write_ptr;
	char*			m_read_buffer;
	char*			m_read_ptr;
	int				m_num_bytes_waiting;

	
	Flags< int >	m_status;
	Flags< int >	m_flags;
    
	App*			m_app;	
	Tmr::Time		m_last_comm_time;	// The last time we've communicated with this connection
    int				m_last_send_time;
	int				m_send_interval;
public:
	enum Status
	{
		mSTATUS_READY 			= 0x0001,
		mSTATUS_BUSY			= 0x0002,
		mSTATUS_INVALID			= 0x0004,
		mSTATUS_DISCONNECTING	= 0x0008
	};

	enum
	{
		mLOCAL		= 0x01,	// local connection: i.e. on this machine
		mREMOTE		= 0x02,	// remote connection: i.e. on another machine
		mFOREIGN	= 0x04,	// foreign connection: i.e. one we don't recognize
	};

	enum
	{   
		vHANDLE_INVALID	= 0,
		vHANDLE_FIRST 	= 1,
	};

	enum
	{
#ifdef __PLAT_WN32__
		vREAD_BUFFER_LENGTH = 10*MAX_UDP_PACKET_SIZE // Used to be 2048
#else
		vREAD_BUFFER_LENGTH = 2*MAX_UDP_PACKET_SIZE // Used to be 2048
#endif
	};

	enum
	{
		vBROADBAND,
		vNARROWBAND,
		vLAN,
	};

	Conn( int flags );
	~Conn( void );

	void			DestroyAllMessageData( void );
    void			AckAllMessages( void );
	void			DestroyMessageQueues( void );
	void			DestroyImportantMessageQueues( void );

	int				GetHandle( void );
	unsigned short	GetPort( void );
	int				GetIP( void );
	int				GetResendThreshold( void );
	Tmr::Time		GetLastCommTime( void );
	Tmr::Time		GetTimeElapsedSinceCommunication( void );
	void			UpdateCommTime( Tmr::Time extra_time = 0 );
	
	void			SetStatus( int status_mask );
	void			ClearStatus( int status_mask );
	bool			TestStatus( int status_mask );
	int				GetStatus( void );

	void			SetIP( int ip );
	void			SetPort( unsigned short port );

	void			Invalidate( void );
	bool			IsValid( void );
	bool			IsForeign( void );
	bool			IsLocal( void );
	bool			IsRemote( void );
    
	void			SetSendInterval( int interval );		// in milliseconds
	int				GetSendInterval( void );
	void			SetForceSendThisFrame( bool force_send );
	bool			GetForceSendThisFrame( void );
	int				GetBandwidthType( void );
    void			SetBandwidthType( int type );

	int				GetAveLatency( void );

	Metrics*		GetOutboundMetrics( void );
	Metrics*		GetInboundMetrics( void );

	void			SetBandwidth( int in_bytes_per_sec );
	int				GetBandwidth( void );

	void			ClearNumResends( void );
	int				GetNumResends( void );
	void			FlagMessagesForResending( unsigned int packet_stamp );

	int				GetNumPendingStreamMessages( void );

	unsigned int	m_SequenceId[MAX_SEQ_GROUPS];			// per-group, per-client array of 'next' sequence ids		
	unsigned int	m_WaitingForSequenceId[MAX_SEQ_GROUPS];	// per-group, per-client array of 'waiting for' sequence ids			
	unsigned int	m_NextStreamId;			// Next stream id
	Lst::Head< MsgSeqLink >	m_SequencedBuffer[MAX_SEQ_GROUPS];	
	Lst::Head< StreamLink > m_StreamInList;
	Lst::Head< StreamLink > m_StreamOutList;
    
private:
	void			destroy_pending_sequenced_messages( void );

	LatencyTest 	m_latency_test;
	Metrics			m_metrics_in;
	Metrics			m_metrics_out;
	bool			m_force_send_packet;
	Lst::Node< Conn > m_node;
	Conn*			m_alias_connection;
	int				m_bandwidth_type;
	unsigned int 	m_latest_packet_stamp;
	unsigned int 	m_latest_sent_packet_stamp;
	int				m_bandwidth;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class Dispatcher
{	
	friend class App;

public:
	Dispatcher( App* app ) 
		: m_app( app ) {}
	MsgHandler*	AddHandler( unsigned char net_msg_id, MsgHandlerCode *code, int flags = 0, 
						void *data = NULL, int pri = NORMAL_PRIORITY );
	void		Init( void );
	void		Deinit( void );
	int			DispatchMsgHandlers( Conn *conn, int flags );
	int			DispatchMessage( MsgHandlerContext *context );

private:
	Lst::Head< MsgHandler > m_handler_list[MAX_NET_MSG_ID];
	App*		m_app;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class  Manager  : public Spt::Class
{
	
public:
	enum
	{
		vMAX_MSG_NAME_LEN	= 32,
		vMAX_PACKET_SIZE 	= MAX_UDP_PACKET_SIZE,
		vMIN_PACKET_SIZE	= 3,		// msg should be sizeof( crc) plus at least 1 byte of data
		vPORT_ANY 			= 0,
		vSTANDARD_RESEND_THRESHOLD 	= 2000,	// ms
		vMINIMUM_RESEND_THRESHOLD 	= 500,		// ms
		vMSG_HEADER_LENGTH 			= 1,
		vMSG_HEADER_LENGTH_WITH_SIZE= 3,
		vMSG_SEQ_HEADER_LENGTH	= 6,
		vMSG_CRC_LEN = 2,
		vMAX_PAYLOAD = vMAX_PACKET_SIZE - vMSG_CRC_LEN,
	};

	void		SetError( int error );
	int			GetLastError( void );

	bool		ConnectToInternet( void );
	bool		DisconnectFromInternet( void );
	bool		IsOnline( void );

	Server*		CreateNewAppServer( int id, char* appName, int max_clients, unsigned short port, 
									int address, int flags = 0 );
	Client*		CreateNewAppClient( int id, char* appName, unsigned short port, int address,
									int flags = 0 );
	
	void		DestroyApp( App *app );
		
	void		SetConnectionType( ConnType conn_type );
	void		SetDeviceType( DeviceType dev_type );
	void		SetPublicIP( unsigned int ip );
	void		SetLocalIP( char* ip );
	void		SetGateway( char* ip );
	void		SetSubnetMask( char* ip );
	void		SetISPPhoneNumber( char* phone_no );
	void		SetISPLogin( char* login );
	void		SetISPPassword( char* password );
	void		SetDHCP( bool use_dhcp );
	void		SetDialupAuthentication( bool authenticate );
	void		SetDNSServer( int index, char* ip );
	void		SetHostName( char* host );
	void		SetDomainName( char* host );
	ConnType	GetConnectionType( void );
	DeviceType	GetDeviceType( void );
	char*		GetLocalIP( void );
	unsigned int	GetPublicIP( void );
	char*		GetGateway( void );
	char*		GetSubnetMask( void );
	char*		GetDNSServer( int index );
	char*		GetISPPhoneNumber( void );
	char*		GetHostName( void );
	char*		GetDomainName( void );
	bool		ShouldUseDialupAuthentication( void );
	bool		ShouldUseDHCP( void );

	Server*		FirstServer( Lst::Search< App > *sh );
	Client*		FirstClient( Lst::Search< App > *sh );
	App*		NextApp( Lst::Search< App > *sh );
	int			NumApps( void );

	void		SetMessageName( unsigned char msg_id, char* msg_name );
	char*		GetMessageName( unsigned char msg_id );
	void		SetMessageFlags( unsigned char msg_id, char flags );
	char		GetMessageFlags( unsigned char msg_id );

	void		AddLogicTasks( App* app );
	void		AddLogicPushTasks( App* app );
	void		RemoveNetworkTasks( App* app );

	bool		NeedToTestNetworkEnvironment( void );
	bool		NetworkEnvironmentSetup( void );

	int			GetModemState( void );
	void		SetModemState( int state );
	int			GetModemError( void );
	int			GetModemBaudRate( void );

	void		SetBandwidth( int bytes_per_sec );
	int			GetBandwidth( void );

	bool		CanChangeDevices( void );
	
protected:	
	Lst::Head< App >	m_net_servers;
	Lst::Head< App >	m_net_clients;
	
private:
	Manager();
	~Manager();

#ifdef NET_DEBUG_MESSAGES
	char		m_message_names[ MAX_MSG_IDS ][ vMAX_MSG_NAME_LEN ];	// Text names for each message
#endif
	char		m_message_flags[ MAX_MSG_IDS ];
	ConnType	m_conn_type;
	DeviceType	m_device_type;
	char		m_local_ip[16];
	unsigned int	m_public_ip;
	char		m_dns_servers[3][16];
	char		m_gateway[16];
	char		m_subnet[16];
	char		m_isp_phone_no[32];
	char		m_isp_user_name[80];
	char		m_isp_password[80];
	char		m_host_name[32];
	char		m_domain_name[32];
	int			m_num_apps;
	int			m_last_error;

	bool		m_online;
	bool		m_use_dhcp;
	bool		m_use_dialup_auth;
	int			m_modem_state;
	int			m_modem_err;	
	int			m_modem_baud_rate;
	int			m_bandwidth;
#ifdef __PLAT_NGC__
	bool		initialize_ngc( void );
#endif
#ifdef __PLAT_NGPS__
	bool		initialize_ps2( void );
	bool		sn_stack_setup( void );
	bool		setup_ethernet_params( void );
	bool		initialize_device( void );
	bool		load_irx_files( void );
	bool		start_stack( void );
	bool		stop_stack( void );
	bool		m_stack_setup;
	bool		m_net_drivers_loaded;
	bool		m_options_changed;
	bool		m_device_changed;
    

	// Sn Modem Setup
	static 	void 			conn_modem_state_callback( sn_int32 modem_state );
	static 	void 			disconn_modem_state_callback( sn_int32 modem_state );
	char					m_modem_thread_stack[ vMODEM_THREAD_STACK_SIZE ]	__attribute__ ((aligned(16)));
	static 	void			threaded_modem_conn( void *data );
	static 	void			threaded_modem_disconn( void *data );
	Thread::PerThreadStruct	m_modem_thread_data;
	Thread::THREADID		m_modem_thread_id;

#endif // __PLAT_NGPS__
    
	DeclareSingletonClass( Manager );
};

class BitStream
{
public:
	BitStream( void );

	void			SetInputData( char* data, int size );
	void			SetOutputData( char* data, int size );

	void			WriteValue( int value, int num_bits );
	void			WriteFloatValue( float value );
	void 			Flush( void );
	float			ReadFloatValue( void );
	unsigned int	ReadUnsignedValue( int num_bits );
	int				ReadSignedValue( int num_bits );

	int				GetByteLength( void );

private:
	unsigned int*	m_data;
	unsigned int*	m_start_data;
	unsigned int	m_cur_val;
	int				m_bits_left;
	int				m_size;
	int				m_bits_processed;
};

class MsgDesc
{
public:
	MsgDesc( void );

	unsigned char	m_Id;
	char			m_StreamMessage;	// bool
	unsigned short	m_Length;
	void*			m_Data;
	int				m_Priority;
	QUEUE_TYPE		m_Queue;
	unsigned char	m_GroupId;
    bool			m_Singular;
	int				m_Delay;
	unsigned int	m_ForcedSequenceId;

};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class BannedConn : public Lst::Node< BannedConn >
{
public:
	BannedConn( void ) : Lst::Node< BannedConn > ( this ) {}
	int	m_Ip;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

class App
{
friend class Manager;

public:
	enum
	{
		mBROADCAST				= 0x01,
		mALIAS_SUPPORT			= 0x02,
		mDYNAMIC_RESEND			= 0x04,
		mLOCAL					= 0x08,	// Local client (i.e. no real port/address info associated with it)
		mACCEPT_FOREIGN_CONN	= 0x10,
		mSECURE					= 0x20,
	};
    
	App( int flags = 0 );
	virtual	~App( void );

	static	const	unsigned int	MAX_LATENCY;
	typedef void	(ForeignPacketHandlerCode)( char* packet, int len, struct sockaddr* sender );
    
	Conn	*GetConnectionByHandle( int handle );
	Conn	*GetConnectionByAddress( int ip, unsigned short port );
	Conn	*NewConnection( int ip, unsigned short port, int flags = Conn::mREMOTE );
	void	BanConnection( Conn* conn );
	bool	IsConnectionBanned( Conn* conn );

	void	StreamMessageToConn( Net::Conn* conn, unsigned char msg_id, unsigned short msg_len, void* data, char* desc, unsigned char group_id = GROUP_ID_DEFAULT,
								 bool all_at_once = true, bool send_in_place = false );
	void	StreamMessage( int handle, unsigned char msg_id, unsigned short msg_len, void* data, char* desc, unsigned char group_id = GROUP_ID_DEFAULT,
						   bool all_at_once = true, bool send_in_place = false );
	void	StreamMessageToServer( unsigned char msg_id, unsigned short msg_len, void* data, char* desc, unsigned char group_id = GROUP_ID_DEFAULT,
								   bool all_at_once = true, bool send_in_place = false );
	void	EnqueueMessage( int handle, MsgDesc* desc );
	void	EnqueueMessageToServer( MsgDesc* desc );
	void 	DequeueMessagesByType( Net::Conn* conn, QUEUE_TYPE queue, unsigned char msg_id );
	void	FreeConnMessageQueue( Conn *conn, QUEUE_TYPE queue );
	void	FreeMessageQueue( QUEUE_TYPE queue );
	bool	BuildMsgStream( Conn *conn, QUEUE_TYPE queue, bool resends_only = false );
	QueuedMsgSeq*	AllocateNewSeqMessage( unsigned char msg_id, unsigned short msg_len, char* data, unsigned char group_id );
	bool	SendMessageTo( unsigned char msg_id, unsigned short msg_len, void* data,
							int ip, unsigned short port, int flags );
	bool	SendTo( int ip, unsigned short port, char *data, int len, int flags );
	bool	Send( char *data, int len, int flags );
	
	Conn	*FirstConnection( Lst::Search< Conn > *sh );
	Conn	*NextConnection( Lst::Search< Conn > *sh );	
	void	TerminateConnection( Conn* conn );

	Manager*	GetManager( void );
	
	int		GetID( void );
	char*	GetName( void );

	bool	IsLocal( void );
	
	void	AcceptForeignConnections( bool accept );
	bool	AcceptsForeignConnections( void );

	bool	ShouldSendThisFrame( Conn* conn );
	bool	MessagesToProcess( Conn* conn );	
	bool	MessagesToSend( Conn *conn );
	bool	ImportantMessagesToSend( Conn* conn );

	int		BandwidthUsed( void );
	bool	BandwidthExceeded( Conn* conn );

	void	ReportError( void );

	void	AliasConnections( Conn* server_conn, Conn* client_conn );

	Tsk::BaseTask&	GetSendDataTask( void );
	Tsk::BaseTask&	GetReceiveDataTask( void );
	Tsk::BaseTask&	GetProcessDataTask( void );
	Tsk::BaseTask&	GetNetworkMetricsTask( void );
	    
	virtual	void	ProcessData( void );
    
	void			SendData( bool scheduled_send = false );
	virtual	void	ReceiveData( void ) = 0;	

	void	SendEnqueuedMessages( void );
	virtual void	SendEnqueuedMessages( Conn* conn ) = 0;
	
	void			ShutDown( void );
    		
	SOCKET			GetSocket( void );
	void			SetForeignPacketHandler( ForeignPacketHandlerCode* code );

	unsigned int	m_Timestamp;
	Dispatcher		m_Dispatcher;

	unsigned int m_TotalBytesOut;
	unsigned int m_TotalBytesIn;
	unsigned int m_LostPackets;
	unsigned int m_LatePackets;
	unsigned int m_DupePackets;
	unsigned int m_FrameCounter;

#ifdef __PLAT_XBOX__
	XNADDR		m_XboxAddr;
	XNKID		m_XboxKeyId;
	XNKEY		m_XboxKey;
	BYTE		m_Nonce[8];
#endif

protected:

	virtual bool	init( void );
	virtual	void	deinit( void );

	bool			bind_app_socket( int address, unsigned short port );

	Lst::Node< App >	m_node;
	SOCKET		m_socket;
	struct	sockaddr_in	m_local_address;
	bool	m_connected;
	
	Manager *m_net_man;	

	Tsk::Task< App >*	m_process_network_data_task;
	Tsk::Task< App >*	m_send_network_data_task;
	Tsk::Task< App >*	m_receive_network_data_task;        
	Tsk::Task< App >*	m_network_metrics_task;

    void    process_stream_messages( void );
	void	terminate_invalid_connections( void );
	void	crc_and_copy_stream( char* in_stream, char* out_stream, int in_len, int* out_len );
	bool	validate_and_copy_stream( char* in_stream, char* out_stream, int in_len );

    int 	m_num_connections;
	int 	m_max_connections;

	static	MsgHandlerCode	handle_sequenced_messages; 	
	static	MsgHandlerCode	handle_connection_request;
	static	MsgHandlerCode	handle_connection_accepted;
	static	MsgHandlerCode	handle_stream_messages; 	

	char	m_out_packet_buffer[ Manager::vMAX_PACKET_SIZE ];
	char	m_in_packet_buffer[ Manager::vMAX_PACKET_SIZE ];

	Flags< int > m_flags;

	ForeignPacketHandlerCode*	m_foreign_handler;
#ifdef __PLAT_NGPS__

public:
	void			TransferData( void );
	
	void			WaitForAsyncCallsToFinish( void );
	void			WaitForTransferSemaphore( void );
	void			SignalTransferSemaphore( void );

protected:
	char	m_socket_thread_stack[ vSOCKET_THREAD_STACK_SIZE ]	__attribute__ ((aligned(16)));

	bool	m_socket_thread_active;

	Thread::THREADID	m_socket_thread_id;
    
	int		m_transfer_semaphore_id;
	int		m_send_semaphore_id;
	int		m_receive_semaphore_id;
	int 	m_active_semaphore_id;
	
	Thread::PerThreadStruct	m_socket_thread_data;
		
	static 	void	threaded_transfer_data( void *data );

	bool	m_shutting_down;
#endif // __PLAT_NGPS__

	static	Tsk::Task< App >::Code	process_network_data;
	static	Tsk::Task< App >::Code	send_network_data;
	static	Tsk::Task< App >::Code	read_network_data;
	static	Tsk::Task< App >::Code	service_network_metrics;
    
private:

	void	process_sequenced_messages( Conn *conn );   
    
	int	get_free_handle( void );
	
    Lst::Head< Conn >		m_connections;
	Lst::Head< BannedConn >	m_banned_connections;
	
	int		m_id;
	char	m_name[MAX_LEN_APP_NAME];
	bool	m_loop_read;
	double	m_double_timestamp;
};

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	unsigned short	Conn::GetPort( void )
{
	return m_port;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	int		Conn::GetIP( void )
{
	return m_ip;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	int Conn::GetHandle( void )
{
	return m_handle;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	int Conn::GetAveLatency( void )
{
	if( IsLocal())
	{
		return 0;
	}
	return m_latency_test.m_AveLatency;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void	Conn::SetIP( int ip )
{
	m_ip = ip;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	void	Conn::SetPort( unsigned short port )
{
	m_port = port;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void		Conn::SetStatus( int status_mask )
{
	m_status.SetMask( status_mask );
}
	
/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline void		Conn::ClearStatus( int status_mask )
{
	m_status.ClearMask( status_mask );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline bool		Conn::TestStatus( int status_mask )
{
	return m_status.TestMask( status_mask );
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline int		Conn::GetStatus( void )
{
	return m_status;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	Metrics*	Conn::GetInboundMetrics( void )
{
	return &m_metrics_in;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/

inline	Metrics*	Conn::GetOutboundMetrics( void )
{
	return &m_metrics_out;
}

/******************************************************************/
/*                                                                */
/*                                                                */
/******************************************************************/


}	// namespace Net


/*****************************************************************************
**								Inline Functions							**
*****************************************************************************/

#ifdef __PLAT_XBOX__
// Need this to be outside the Net:: namespace...
inline char* inet_ntoa( struct in_addr addr )
{
	IN_ADDR		in_a;
	in_a.S_un	= addr.S_un;

	const int	STRING_BUFFER_SIZE = 100;
//	static char	string_buffer[STRING_BUFFER_SIZE];
//	int			rv = XNetInAddrToString( in_a, string_buffer, STRING_BUFFER_SIZE );

	// Cheesy hack for now.
	static char	string_buffer[STRING_BUFFER_SIZE] = "0.0.0.0";

	return string_buffer;
}
#endif

#endif // __NET_H__

