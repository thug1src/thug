/****************************************************************************/
/*                                                                          */
/* Copyright SN Systems Ltd 2003                                            */
/*                                                                          */
/* File:        netglue_sneebsd.c                                           */
/* Version:     1.01                                                        */
/* Description: Netglue implementation file for SN NDK EE BSD API           */
/*                                                                          */
/* Change History:                                                          */
/* Vers Date        Author     Changes                                      */
/* 1.00 19-May-2003 D.Lowther  File Created (for use with DNAS - LN 13419)  */
/* 1.01 02-Jun-2003 D.Lowther  Fixed bug in sceNetGlueSocket() where it was */
/*                             checking for iRetval == 0, should be > 0     */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include <core/defines.h>
#include <stdio.h>
#include <eetypes.h>
#include <eekernel.h>
#include <snsocket.h>
#include <libgraph.h>
#include <string.h>

/****************************************************************************/
/* Stuff that should come from netglue header files but including netglue.h */
/* causes clashes with snsocket.h                                           */
/****************************************************************************/

extern "C" {

typedef long ssize_t;

typedef u_char sceNetGlueSaFamily_t;
typedef u_int  sceNetGlueSocklen_t;

#define sa_family_t sceNetGlueSaFamily_t
#define socklen_t   sceNetGlueSocklen_t

typedef struct sceNetGlueSockaddr 
{
    u_char      sa_len;
    sa_family_t sa_family;
    char        sa_data[14];
} sceNetGlueSockaddr_t;

typedef struct sceNetGlueHostent
{
    char    *h_name;
    char    **h_aliases;
    int     h_addrtype;
    int     h_length;
    char    **h_addr_list;
#define h_addr h_addr_list[0]
} sceNetGlueHostent_t;

typedef struct sceNetGlueInAddr 
{
    u_int s_addr;
} sceNetGlueInAddr_t;

/****************************************************************************/
/* The netglue error codes in the 2.7 lib ref are wrong, Sony's advice is   */
/* to use the values from ee/gcc/ee/include/sys/errno.h, but I can't        */
/* include that file in here because of clashes with the same names in      */ 
/* snsocket.h                                                               */ 
/* So the following values are copied / pasted from the above errno.h file  */
/* and I've stuck NG_ on the front of the names, they are listed in the     */
/* same order as in the 2.7 lib ref for netglue (not numerical order)       */
/****************************************************************************/

#define NG_EBADF           9 /* Invalid descriptor specified                */
#define NG_ENOMEM         12 /* Insufficient memory                         */
#define NG_EBUSY          16 /* Library not yet available (before           */
                             /* initialization completes, etc.)             */
#define NG_EINVAL         22 /* Invalid arguments specified                 */
#define NG_EPROTOTYPE    107 /* Unsupported protocol type specified         */
#define NG_EOPNOTSUPP     95 /* Invalid call for socket                     */
#define NG_EPFNOSUPPORT   96 /* Unsupported protocol family specified       */
#define NG_EAFNOSUPPORT  106 /* Specified address family is an unsupported  */
                             /* value for the socket protocol family        */
#define NG_EADDRINUSE    112 /* Tried to bind to port already bound         */
#define NG_EADDRNOTAVAIL 125 /* Invalid address specified                   */
#define NG_ENETDOWN      115 /* Interface is down                           */
#define NG_EHOSTUNREACH  118 /* Network is unreachable                      */
#define NG_ECONNABORTED  113 /* Aborted by call to sceGlueAbort()           */
#define NG_ECONNRESET    104 /* Connection was reset                        */
#define NG_EISCONN       127 /* Specified connection has already been made  */
#define NG_ENOTCONN      128 /* Specified connection does not exist         */
#define NG_ETIMEDOUT     116 /* Timeout                                     */
#define NG_ECONNREFUSED  111 /* Connection request was refused              */

/* NDK can return other error codes e.g to indicate problems with the       */
/* comms between the EE and IOP, where there isn't an obvious map from NDK  */
/* to Netglue error code, I'll use NG_UNSPECIFIED                           */ 

#define NG_UNSPECIFIED    NG_EINVAL /* Don't know what else to use          */

/****************************************************************************/
/* Netglue error codes for DNS operations from netdb.h + NG_ on front       */
/****************************************************************************/

#define NG_NETDB_INTERNAL -1 /* see errno */
#define NG_NETDB_SUCCESS  0  /* no problem */
#define NG_HOST_NOT_FOUND 1  /* Authoritative Answer Host not found */
#define NG_TRY_AGAIN      2  /* Non-Authoritative Host not found, or SERVERFAIL */
#define NG_NO_RECOVERY    3  /* Non recoverable errs, FORMERR, REFUSED, NOTIMP */
#define NG_NO_DATA        4  /* Valid name, no data record of requested type */
#define	NG_NO_ADDRESS   NG_NO_DATA /* no address, look for MX record */


/****************************************************************************/
/* Private data / types                                                     */
/****************************************************************************/

/* Absolute maximum number of EE threads which may use EE socket API */
#define ABS_MAX_THREADS      10

/* Structure to hold info for each thread using this netglue API */

typedef struct snNetGlueThreadInfo 
{
    int                 iErrno;
    int                 iHerrno;
    int                 iThreadId;
    sceNetGlueHostent_t NetGlueHostent;
    /* Following are for sceNetGlueGethostbyaddr */
    sn_uint32           *sn_h_addr_list[2]; /* one plus null terminator */
    sn_uint32           sn_h_addr;          /* above [0] points to this */
} snNetGlueThreadInfo_t;

/* Array to hold info for all the threads using this netglue API */
/* An element is 'free' if it's iThreadId is zero                */

static snNetGlueThreadInfo_t s_ThreadInfo[ABS_MAX_THREADS] =

{{ 0, 0, 0, {0,0,0,0,0}, {0,0}, 0},
 { 0, 0, 0, {0,0,0,0,0}, {0,0}, 0}, 
 { 0, 0, 0, {0,0,0,0,0}, {0,0}, 0},
 { 0, 0, 0, {0,0,0,0,0}, {0,0}, 0},
 { 0, 0, 0, {0,0,0,0,0}, {0,0}, 0},
 { 0, 0, 0, {0,0,0,0,0}, {0,0}, 0},
 { 0, 0, 0, {0,0,0,0,0}, {0,0}, 0},
 { 0, 0, 0, {0,0,0,0,0}, {0,0}, 0},
 { 0, 0, 0, {0,0,0,0,0}, {0,0}, 0},
 { 0, 0, 0, {0,0,0,0,0}, {0,0}, 0}};

/****************************************************************************/
/* Private functions                                                        */
/****************************************************************************/

/****************************************************************************/
/* Private functions - for accessing s_ThreadInfo                           */
/****************************************************************************/

static int snGetThreadInfoIndexForThreadId(int iThreadId)
{

    /* Searches s_ThreadInfo[].iThreadId for a match with iThreadId         */
    /* If match found returns index in s_ThreadInfo, else returns -1        */

    int i;

    for (i=0; i<ABS_MAX_THREADS; i++)
    {
        if (s_ThreadInfo[i].iThreadId == iThreadId)
        {
            return i;
        }
    }

    return -1;
}

static int snGetThreadInfoIndexFreeEntry(void)
{
    /* Searches s_ThreadInfo[].iThreadId for iThreadId == 0 (free entry)    */
    /* If free entry found returns index in s_ThreadInfo, else returns -1   */

    int iRetval;

    iRetval = snGetThreadInfoIndexForThreadId(0);

    return iRetval;
}

static int snAddToThreadInfo(int iThreadId)
{
    /* Checks that iThreadId is not already stored in s_ThreadInfo, then    */
    /* looks for a free entry and stores it in there.                       */
    /* If there is no space to store iThreadId, returns -1, else returns    */
    /* the index where iThreadId is stored in s_ThreadInfo                  */

    int iInterruptsWereEnabled;
    int iThreadInfoIndex;

    /* Do this with interrupts disabled to serialise access */
    iInterruptsWereEnabled = DI();

    /* If this thread is already registered print a warning and return ok   */
    iThreadInfoIndex = snGetThreadInfoIndexForThreadId(iThreadId);
    if (iThreadInfoIndex >= 0)
    {
        if(iInterruptsWereEnabled != 0)
        {
            EI();
        }
        printf("Warning: snAddToThreadInfo() was already added\n");
        return iThreadInfoIndex;
    }

    /* Not already registered, so try and find a spare entry to use */
    iThreadInfoIndex = snGetThreadInfoIndexFreeEntry();

    /* If the list is full, print error message and return error */
    if (iThreadInfoIndex < 0)
    {
        if(iInterruptsWereEnabled != 0)
        {
            EI();
        }
        printf("Error: snAddToThreadInfo() list is full\n");
        return -1;
    }

    /* Store iThreadId in s_ThreadInfo */
    s_ThreadInfo[iThreadInfoIndex].iThreadId = iThreadId;
    s_ThreadInfo[iThreadInfoIndex].iErrno    = 0;
    s_ThreadInfo[iThreadInfoIndex].iHerrno   = 0;

    if(iInterruptsWereEnabled != 0)
    {
        EI();
    }
    return iThreadInfoIndex;
}

static int snRemoveFromThreadInfo(int iThreadId)
{
    /* Finds iThreadId in s_ThreadInfo and then sets it to zero             */
    /* If it didn't find iThreadId returns -1, else returns index           */

    int iInterruptsWereEnabled;
    int iThreadInfoIndex;

    /* Do this with interrupts disabled to serialise access */
    iInterruptsWereEnabled = DI();

    /* Search s_ThreadInfo for iThreadId */
    iThreadInfoIndex = snGetThreadInfoIndexForThreadId(iThreadId);
    if (iThreadInfoIndex >= 0)
    {
        /* Mark the entry where it was stored as free now */
        s_ThreadInfo[iThreadInfoIndex].iThreadId = 0;
    }

    if(iInterruptsWereEnabled != 0)
    {
        EI();
    }
    return iThreadInfoIndex;
}


/****************************************************************************/
/* Private functions - for converting / storing error codes                 */
/****************************************************************************/

static void snStoreErrno(int iErrno)
{

    /* iErrno must be a netglue error code, this function will store it     */
    /* in s_ThreadInfo[].iErrno for the entry for this ThreadId             */

    int iThreadInfoIndex;

    /* Get index in s_ThreadInfo[] for this thread */
    iThreadInfoIndex = snGetThreadInfoIndexForThreadId(GetThreadId());

    /* This shouldn't happen, but be defensive */
    if (iThreadInfoIndex < 0)
    {
        printf("Warning: snStoreErrno() thread not registered\n");
        return;
    }
    
    s_ThreadInfo[iThreadInfoIndex].iErrno = iErrno;
}

static int snConvertNdkErrToNetGlueErr(int iSnErrno)
{
    /* Converts NDK error number in iSnErrno into the closest NetGlue error */
    /* number and returns it                                                */

    int iRetval = 0;

    switch(iSnErrno)
    {
        case ENOBUFS:                  /* No memory buffers are available   */
            iRetval = NG_ENOMEM;       /* Insufficient memory               */
        break;

        case ETIMEDOUT:                /* Operation timed out               */
            iRetval = NG_ETIMEDOUT;    /* Timeout                           */
        break;

        case EISCONN:                  /* The socket is already connected   */
            iRetval = NG_EISCONN;      /* Connection has already been made  */
        break;

        case EOPNOTSUPP:               /* Operation not supported           */
            iRetval = NG_EOPNOTSUPP;   /* Invalid call for socket           */
        break;

        case ECONNABORTED:             /* The connection was aborted        */
            iRetval = NG_ECONNABORTED; /* Aborted by call to sceGlueAbort() */
        break;

        case EWOULDBLOCK:              /* Caller would block                */
            iRetval = -1;              /* Shouldn't happen in blocking mode */
        break;

        case ECONNREFUSED:             /* The connection was refused        */
            iRetval = NG_ECONNREFUSED; /* Connection request was refused    */
        break;

        case ECONNRESET:               /* The connection has been reset     */
            iRetval = NG_ECONNRESET;   /* Connection was reset              */
        break;

        case ENOTCONN:                 /* The socket is not connected       */
            iRetval = NG_ENOTCONN;     /* Connection does not exist         */
        break;

        case EALREADY:                 /* Operation is already in progress  */
            iRetval = -1;              /* Shouldn't happen in blocking mode */
        break;

        case EINVAL:                   /* Invalid parameter                 */
            iRetval = NG_EINVAL;       /* Invalid arguments specified       */
        break;

        case EMSGSIZE:                 /* Invalid message size              */
            iRetval = -1;              /* Shouldn't happen (not using fn's) */              
        break;

        case EPIPE:                    /* Cannot send any more              */
                                       /* Closest NG error code is ...      */
            iRetval = NG_ENOMEM;       /* Insufficient memory               */
        break;

        case EDESTADDRREQ:             /* Destination address is missing    */
                                       /* Closest NG error code is ...      */
            iRetval = NG_EINVAL;       /* Invalid arguments specified       */
        break;

        case ESHUTDOWN:                /* Connection has been shut down     */
                                       /* Closest NG error code is ...      */
            iRetval = NG_ENOTCONN;     /* Connection does not exist         */
        break;

        case ENOPROTOOPT:              /* Option unknown for this protocol  */
                                       /* Closest NG error code is ...      */
            iRetval = NG_EINVAL;       /* Invalid arguments specified       */
        break;

        case EHAVEOOB:                 /* Have received out-of-band data    */
            iRetval = -1;              /* Shouldn't happen                  */              
        break;

        case ENOMEM:                   /* No memory available               */
            iRetval = NG_ENOMEM;       /* Insufficient memory               */
        break;

        case EADDRNOTAVAIL:            /* Specified address not available   */
            iRetval = NG_EADDRNOTAVAIL;/* Invalid address specified         */
        break;

        case EADDRINUSE:               /* Specified address already in use  */
            iRetval = NG_EADDRINUSE;   /* Tried bind to port already bound  */
        break;

        case EAFNOSUPPORT:             /* Address family is not supported   */
            iRetval = NG_EAFNOSUPPORT; /* Address family is an unsupported  */
        break;

        case EINPROGRESS:              /* Operation is in progress          */
            iRetval = -1;              /* Shouldn't happen in blocking mode */
        break;

        case ELOWER:                   /* Unused                            */
            iRetval = -1;              /* Shouldn't happen                  */
        break;

        case EBADF:                    /* Socket descriptor is invalid      */
            iRetval = NG_EBADF;        /* Invalid descriptor specified      */
        break;

        case SN_ENOTINIT:              /* Socket API not initialised        */
                                       /* Closest NG error code is ...      */
            iRetval = NG_EBUSY;        /* Library not yet available         */
        break;

        case SN_ETHNOTREG:             /* Thread not registered with API    */
                                       /* Closest NG error code is ...      */
            iRetval = NG_EINVAL;       /* Invalid arguments specified       */
        break;

        case SN_EMAXTHREAD:            /* Max no of threads exceeded        */
                                       /* Closest NG error code is ...      */
            iRetval = NG_EINVAL;       /* Invalid arguments specified       */
        break;

        case SN_EIOPNORESP:            /* Failed to initialise IOP end      */
                                       /* Closest NG error code is ...      */
            iRetval = NG_EBUSY;        /* Library not yet available         */
        break;

        case SN_ENOMEM:                /* Not enough mem to init sock API   */
                                       /* Closest NG error code is ...      */
            iRetval = NG_ENOMEM;       /* Insufficient memory               */
        break;

        case SN_EBINDFAIL:             /* sceSifBindRpc failed              */
                                       /* Closest NG error code is ...      */
            iRetval = NG_EBUSY;        /* Library not yet available         */
        break;

        case SN_EINVTHREAD:            /* Invalid thread id                 */
                                       /* Closest NG error code is ...      */
            iRetval = NG_EINVAL;       /* Invalid arguments specified       */
        break;

        case SN_EALRDYINIT:            /* Socket API already initialised    */
            iRetval = -1;              /* Shouldn't happen                  */
        break;

        case SN_ESTKDOWN:              /* Stack has not been started        */
                                       /* Closest NG error code is ...      */
            iRetval = NG_EBUSY;        /* Library not yet available         */
        break;

        case SN_EIFSTATE:              /* Error in inet_ifstate             */
                                       /* Closest NG error code is ...      */
            iRetval = NG_EBUSY;        /* Library not yet available         */
        break;

        case SN_EDNSFAIL:              /* General failure code for DNS      */
            iRetval = -1;              /* Shouldn't get this for socket err */
        break;

        case SN_SWERROR:               /* Software error in SN sys code     */
            iRetval = -1;              /* Shouldn't happen                  */
        break;

        case SN_EDNSBUSY:              /* DNS is busy try again later       */
            iRetval = -1;              /* Shouldn't get this for socket err */
        break;

        case SN_REQSIZE:               /* RPC request is not expected size  */
            iRetval = -1;              /* Shouldn't happen                  */
        break;

        case SN_REQINV:                /* RPC request is not valid          */
            iRetval = -1;              /* Shouldn't happen                  */
        break;

        case SN_REQSYNC:               /* RPC general IOP s/w failure       */
            iRetval = -1;              /* Shouldn't happen                  */
        break;

        case SN_RPCBAD:                /* RPC fn code unknown to IOP        */
            iRetval = -1;              /* Shouldn't happen                  */
        break;

        case SN_NOTREADY:              /* Device not ready                  */
                                       /* Closest NG error code is ...      */
            iRetval = NG_ENETDOWN;     /* Interface is down                 */
        break;

        default:                       /* Unknown SN NDK error code         */
            iRetval = -1;              /* Shouldn't happen                  */
    }

    /* If iRetval is set to -1 (unexpected error) has to be worth printing  */
    /* a debug error message to indicate what the SN error code was         */

    if (iRetval == -1)
    {
        printf("Warning: snConvertNdkErrToNetGlueErr(%d) unexpected\n",iSnErrno);
        iRetval = NG_UNSPECIFIED;
    }

    return iRetval;
}

static void snGetConvStoreSocketError(int iSockId)
{
    /* This should only be called when an SN NDK function that operates on  */
    /* a socket returns -1 indicating that it failed                        */
    /*                                                                      */
    /* Get the sn error number for the socket specified by iSockId, convert */
    /* to a netglue error code and store it in s_ThreadInfo[].iErrno for    */
    /* this thread id                                                       */

    int iSnErrno;
    int iNgErrno;

    /* Get the NDK error code */
    iSnErrno = sn_errno(iSockId);

    /* Convert it to a netglue error code */
    iNgErrno = snConvertNdkErrToNetGlueErr(iSnErrno);

    /* Store it in s_ThreadInfo[].iErrno for this thread */
    snStoreErrno(iNgErrno);
}

static void snStoreHerrno(int iHerrno)
{
    /* iHerrno must be a netglue error code, this function will store it   */
    /* in s_ThreadInfo[].iErrno for the entry for this ThreadId             */

    int iThreadInfoIndex;

    /* Get index in s_ThreadInfo[] for this thread */
    iThreadInfoIndex = snGetThreadInfoIndexForThreadId(GetThreadId());

    /* This shouldn't happen, but be defensive */
    if (iThreadInfoIndex < 0)
    {
        printf("Warning: snStoreHerrno() thread not registered\n");
        return;
    }
    
    s_ThreadInfo[iThreadInfoIndex].iHerrno = iHerrno;
}

static void snGetConvStoreDnsError(void)
{
    /* This should only be called when an SN NDK function that results in   */
    /* an error that can be read by sn_h_errno()                            */
    /*                                                                      */
    /* Get the error number from sn_h_errno() and convert to a netglue      */
    /* error code and store it in s_ThreadInfo[].iHerrno for this thread id */

    int iSnHerrno;
    int iNgHerrno = 0;

    /* Get the NDK error code */
    iSnHerrno = sn_h_errno();

    /* Convert it to a netglue error code */
    switch (iSnHerrno)
    {
        /* NDK does not support the standard BSD DNS error codes:           */
        /* HOST_NOT_FOUND, TRY_AGAIN, NO_RECOVERY, NO_DATA, NO_ADDRESS, all */
        /* of these conditions result in NDK returning the following error  */

        case SN_EDNSFAIL:
            iNgHerrno = NG_HOST_NOT_FOUND; /* not sure what else I can do */
        break;

        /* Internal errors in the TCP/IP stack state, or, invalid input     */
        /* can result in the following errors:                              */
        /* SN_ESTKDOWN The TCP/IP Stack has not been started                */
        /* EINVAL name was too long or was a NULL pointer                   */

        default:
            printf("Warning: snGetConvStoreDnsError(%d) unexpected\n",iSnHerrno);
            iNgHerrno = NG_NETDB_INTERNAL;
            /* The sony comment for the above Herrno says "see errno", so I */
            /* I assume I should store something in the errno.              */
            snStoreErrno(NG_UNSPECIFIED);
    }

    /* Store the Herrno in s_ThreadInfo[].iHerrno for this thread */
    snStoreHerrno(iNgHerrno);
}

/****************************************************************************/
/* Private functions - convert sceNetGlueSockaddr_t <-> struct sockaddr     */
/****************************************************************************/

static void snConvGlueToNdkSockaddr(sceNetGlueSockaddr_t *psAddr)
{
    /* Does in place conversion                                             */
    /* from: sceNetGlueSockaddr_t                                           */
    /* to:   NDK struct sockaddr                                            */
    
    /* Do nothing if addr is NULL */
    if (psAddr == NULL)
        return;

    /* Swap the byte order for NDK sa_family (16 bits) doesn't have sa_len  */
    psAddr->sa_len    = psAddr->sa_family;
    psAddr->sa_family = 0;
}

static void snConvNdkToGlueSockaddr(sceNetGlueSockaddr_t *psAddr)
{
    /* Does in place conversion                                             */
    /* from: NDK struct sockaddr                                            */
    /* to:   sceNetGlueSockaddr_t                                           */
    
    /* Do nothing if addr is NULL */
    if (psAddr == NULL)
        return;
    
    /* Swap the byte order for NDK sa_family (16 bits) doesn't have sa_len  */
    psAddr->sa_family = psAddr->sa_len;
    psAddr->sa_len    = 16;
}

/****************************************************************************/
/* Private functions - misc                                                 */
/****************************************************************************/

static sceNetGlueHostent_t *snConvNdkToGlueHostent(struct hostent *psNdkHostent)
{
    /* convert hostent from NDK format to NetGlue format and store it in    */
    /* s_ThreadInfo[].NetGlueHostent for this thread                        */

    int iThreadInfoIndex;
    sceNetGlueHostent_t *psRetval = NULL;

    /* Get the index for this thread in s_ThreadInfo[] */
    iThreadInfoIndex = snGetThreadInfoIndexForThreadId(GetThreadId());

    if (iThreadInfoIndex < 0)
    {
        printf("Error: snConvNdkToGlueHostent() unregistered thread\n");
        return NULL;
    }

    /* Found the index, so now convert from NDK struct hostent, into        */
    /* sceNetGlueHostent_t, only difference is that the two fields          */
    /* h_addrtype and h_length are 16 bit in NDK / 32 bit in Netglue        */

    psRetval = &s_ThreadInfo[iThreadInfoIndex].NetGlueHostent;    
    memset(psRetval, 0, sizeof(*psRetval)); /* just being defensive */
    psRetval->h_addr_list = psNdkHostent->h_addr_list;
    psRetval->h_addrtype  = psNdkHostent->h_addrtype;
    psRetval->h_aliases   = psNdkHostent->h_aliases;
    psRetval->h_length    = psNdkHostent->h_length;
    psRetval->h_name      = psNdkHostent->h_name;

    return psRetval;
}

/****************************************************************************/
/* Functions exported by netglue (in same order as sony libref doc)         */
/****************************************************************************/

int *__sceNetGlueErrnoLoc(void)
{
    int iThreadInfoIndex;
    int iThreadId;

    /* Find the index for this thread in s_ThreadInfo */
    iThreadId = GetThreadId();
    iThreadInfoIndex = snGetThreadInfoIndexForThreadId(iThreadId);

    /* This shouldn't happen, but be defensive */
    if (iThreadInfoIndex < 0)
    {
        printf("Error: __sceNetGlueErrnoLoc() unregistered thread (%d)\n",iThreadId);
        return NULL;
    }
    
    return &s_ThreadInfo[iThreadInfoIndex].iErrno;
}

int *__sceNetGlueHErrnoLoc(void)
{
    int iThreadInfoIndex;
    int iThreadId;

    /* Find the index for this thread in s_ThreadInfo */
    iThreadId = GetThreadId();
    iThreadInfoIndex = snGetThreadInfoIndexForThreadId(iThreadId);

    /* This shouldn't happen, but be defensive */
    if (iThreadInfoIndex < 0)
    {
        printf("Error: __sceNetGlueHErrnoLoc() unregistered thread (%d)\n",iThreadId);
        return NULL;
    }
    
    return &s_ThreadInfo[iThreadInfoIndex].iHerrno;
}

int sceNetGlueAbort(int sockfd)
{
    /* The best approximation to this is to close the socket */
    printf("Warning sceNetGlueAbort() just does closesocket\n");
    closesocket(sockfd);
    return 0;
}

int sceNetGlueAbortResolver(int thread_id)
{
    /* Can't see a way to implement this at the moment */
    printf("Warning sceNetGlueAbortResolver() does nothing\n");
    return 0;
}

int sceNetGlueAccept
    (int s, sceNetGlueSockaddr_t *addr, sceNetGlueSocklen_t *paddrlen)
{
    int iRetval;

    /* *addr is an output so no need to convert it before calling NDK */

    /* Call the NDK equivalent */
    iRetval = accept(s, (struct sockaddr*) addr, (int*) paddrlen);

    /* NetGlue and NDK both use positive value for success */
    if (iRetval > 0)
    {
        /* convert *addr from NDK to Netglue format */
        snConvNdkToGlueSockaddr(addr);
        return iRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

int sceNetGlueBind
    (int s, const sceNetGlueSockaddr_t *addr, sceNetGlueSocklen_t addrlen)
{

    int iRetval;

    /* *addr is an input so need to convert it before calling NDK */
    /* but it's a const, so have to clone it before converting it */
    sceNetGlueSockaddr_t copyAddr;
    memcpy(&copyAddr, addr, sizeof(copyAddr));

    /* convert *addr from Netglue to NDK format */
    snConvGlueToNdkSockaddr(&copyAddr);

    /* Call the NDK equivalent */
    iRetval = bind(s, (struct sockaddr*) &copyAddr, addrlen);

    /* NetGlue and NDK both use zero for success */
    if (iRetval == 0)
    {
        return iRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

int sceNetGlueConnect
    (int s, const sceNetGlueSockaddr_t *addr, sceNetGlueSocklen_t addrlen)
{
    int iRetval;

    /* *addr is an input so need to convert it before calling NDK */
    /* but it's a const, so have to clone it before converting it */
    sceNetGlueSockaddr_t copyAddr;
    memcpy(&copyAddr, addr, sizeof(copyAddr));

    /* convert *addr from Netglue to NDK format */
    snConvGlueToNdkSockaddr(&copyAddr);

    /* Call the NDK equivalent */
    iRetval = connect(s, (struct sockaddr*) &copyAddr, addrlen);

    /* NetGlue and NDK both use zero for success */
    if (iRetval == 0)
    {
        return iRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

sceNetGlueHostent_t*
    sceNetGlueGethostbyaddr(const char *addr, int len, int type)
{
    /* NDK does not currently support reverse DNS lookup. Advice from Sony  */
    /* is that DNAS does not use this function (although it does appear in  */
    /* the list of symbols for the DNAS lib)                                */

    /* Without reverse DNS lookup, the best I can do is construct a         */
    /* sceNetGlueHostent_t to return that contains the addr input param and */
    /* and NULL name / alias                                                */

    int iThreadInfoIndex;
    sceNetGlueHostent_t *psRetval = NULL;
    
    iThreadInfoIndex = snGetThreadInfoIndexForThreadId(GetThreadId());

    if (iThreadInfoIndex < 0)
    {
        printf("Error: sceNetGlueGethostbyaddr() unregistered thread\n");
        /* Putting this in Herrno should make app look at Errno */
        snStoreHerrno(NG_NETDB_INTERNAL);
        snStoreErrno(NG_EINVAL);
        return NULL;
    }

    /* Get pointer to sceNetGlueHostent_t for this thread */
    psRetval = &s_ThreadInfo[iThreadInfoIndex].NetGlueHostent;

    /* Set up the sceNetGlueHostent_t that's going to be returned */
    psRetval->h_addr_list    = (char**)&s_ThreadInfo[iThreadInfoIndex].sn_h_addr_list;
    psRetval->h_addr_list[0] = (char*) &s_ThreadInfo[iThreadInfoIndex].sn_h_addr;
    psRetval->h_addr_list[1] = NULL;
    psRetval->h_addrtype     = AF_INET; /* NDK and Netglue define this as 2 */
    psRetval->h_aliases      = NULL;
    psRetval->h_length       = 4;
    psRetval->h_name         = NULL;

    /* Copy the input addr to the sn_h_addr pointed to by above struct */
    memcpy(&s_ThreadInfo[iThreadInfoIndex].sn_h_addr, addr, 4);

    return psRetval;
}

sceNetGlueHostent_t *sceNetGlueGethostbyname(const char *name)
{
    struct hostent *psResult      = NULL;
    sceNetGlueHostent_t *psRetval = NULL;

    /* Call the NDK equivalent */
    psResult = gethostbyname(name);

    /* NetGlue and NDK both use NULL for error */
    if (psResult != NULL)
    {
        /* convert hostent from NDK format to NetGlue format and store it   */
        /* in s_ThreadInfo[].NetGlueHostent for this thread                 */
        psRetval = snConvNdkToGlueHostent(psResult);
        return psRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreDnsError();

    /* NetGlue and NDK both use -1 for error */
    return NULL;
}

int sceNetGlueGetpeername
    (int s, sceNetGlueSockaddr_t *addr, sceNetGlueSocklen_t *paddrlen)
{
    int iRetval;

    /* *addr is an output so no need to convert it before calling NDK */

    /* Call the NDK equivalent */
    iRetval = getpeername(s, (struct sockaddr*) addr, (int*) paddrlen);

    /* NetGlue and NDK both use zero for success */
    if (iRetval == 0)
    {
        /* convert *addr from NDK to Netglue format */
        snConvNdkToGlueSockaddr(addr);
        return iRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

int sceNetGlueGetsockname
    (int s, sceNetGlueSockaddr_t *addr, sceNetGlueSocklen_t *paddrlen)
{
    int iRetval;

    /* *addr is an output so no need to convert it before calling NDK */

    /* Call the NDK equivalent */
    iRetval = getsockname(s, (struct sockaddr*) addr, (int*) paddrlen);

    /* NetGlue and NDK both use zero for success */
    if (iRetval == 0)
    {
        /* convert *addr from NDK to Netglue format */
        snConvNdkToGlueSockaddr(addr);
        return iRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

int sceNetGlueGetsockopt
    (int s, int level, int optname, void *optval,
     sceNetGlueSocklen_t *optlen)
{
    /* At the moment the only option supported by NetGlue is TCP_NODELAY    */
    /* which uses level = IPPROTO_TCP.                                      */
    /* Because the values defined for IPPROTO_TCP and TCP_NODELAY are the   */
    /* same in NDK and Netglue, there is no need to do any conversion here  */

    int iRetval;

    /* Call the NDK equivalent */
    iRetval = getsockopt(s, level, optname, optval, (int*) optlen);

    /* NetGlue and NDK both use zero for success */
    if (iRetval == 0)
    {
        return iRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

u_int sceNetGlueHtonl(u_int hostlong)
{
    return htonl(hostlong);
}

u_short sceNetGlueHtons(u_short hostshort)
{
    return htons(hostshort);
}

u_int sceNetGlueInetAddr(const char *cp)
{
    int iRetval;

    /* Call the NDK equivalent */
    iRetval = inet_addr(cp);

    /* NetGlue and NDK both use INADDR_NONE (all f's) for failure */
    return iRetval;
}

int sceNetGlueInetAton(const char *cp, sceNetGlueInAddr_t *addr)
{
    /* sceNetGlueInAddr_t is the same as NDK struct in_addr */

    int iRetval;

    /* Call the NDK equivalent */
    iRetval = inet_aton(cp, (struct in_addr*) addr);

    /* NetGlue and NDK both use 1 for success / 0 for failure */
    return iRetval;
}

u_int sceNetGlueInetLnaof(sceNetGlueInAddr_t in)
{
    /* to do not implemented yet - but not used by DNAS 2.7.1 */
    printf("Warning: sceNetGlueInetLnaof() not implemented\n");
    return 0;
}

sceNetGlueInAddr_t sceNetGlueInetMakeaddr(u_int net, u_int host)
{
    /* to do not implemented yet - but not used by DNAS 2.7.1 */
    sceNetGlueInAddr_t sRetval;
    memset(&sRetval, 0, sizeof(sRetval));
    printf("Warning: sceNetGlueInetMakeaddr() not implemented\n");
    return sRetval;
}

u_int sceNetGlueInetNetof(sceNetGlueInAddr_t in)
{
    /* to do not implemented yet - but not used by DNAS 2.7.1 */
    printf("Warning: sceNetGlueInetNetof() not implemented\n");
    return 0;
}

u_int sceNetGlueInetNetwork(const char *cp)
{
    /* to do not implemented yet - but not used by DNAS 2.7.1 */
    printf("Warning: sceNetGlueInetNetwork() not implemented\n");
    return INADDR_NONE;
}

char *sceNetGlueInetNtoa(sceNetGlueInAddr_t in)
{
    char *pszRetval;

    /* sceNetGlueInAddr_t is the same as NDK struct in_addr, but can't cast */
    /* non scalar in param passed to call, so copy to ndk_in                */
    struct in_addr ndk_in;
    ndk_in.s_addr = in.s_addr;

    /* Call the NDK equivalent */
    pszRetval = inet_ntoa(ndk_in);

    /* If NDK fails it returns NULL, netglue doc has no mention of failure  */
    return pszRetval;
}

int sceNetGlueListen(int s, int backlog)
{
    int iRetval;

    /* Call the NDK equivalent */
    iRetval = listen(s, backlog);

    /* NetGlue and NDK both use 0 for success */
    if (iRetval == 0)
    {
        return iRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

u_int sceNetGlueNtohl(u_int netlong)
{
    return ntohl(netlong);
}

u_short sceNetGlueNtohs(u_short netshort)
{
    return ntohs(netshort);
}

ssize_t sceNetGlueRecv(int s, void *buf, size_t len, int flags)
{
    int iRetval;

    /* 2.7.0 Netglue doc says flags "Not supported (always set to zero)     */
    /* So enforce this and print error if flags != 0                        */

    if (flags != 0)
    {
        printf("Warning: sceNetGlueRecv() called with flags non zero (%d)\n",flags);
        flags = 0;
    }

    /* Call the NDK equivalent */
    iRetval = recv(s, buf, len, flags);

    /* NetGlue and NDK both use >= 0 for success */
    if (iRetval >= 0)
    {
        return iRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

ssize_t sceNetGlueRecvfrom
    (int s, void *buf, size_t len, int flags,
     sceNetGlueSockaddr_t *addr, sceNetGlueSocklen_t *paddrlen)
{
    int iRetval;

    /* 2.7.0 Netglue doc says flags "Not supported (always set to zero)     */
    /* So enforce this and print error if flags != 0                        */

    if (flags != 0)
    {
        printf("Warning: sceNetGlueRecvfrom() called with flags non zero (%d)\n",flags);
        flags = 0;
    }

    /* *addr is an output so no need to convert it before calling NDK */

    /* Call the NDK equivalent */
    iRetval = recvfrom(s, buf, len, flags, (struct sockaddr*) addr, (int*) paddrlen);

    /* NetGlue and NDK both use >= 0 for success */
    if (iRetval >= 0)
    {
        /* convert *addr from NDK to Netglue format */
        snConvNdkToGlueSockaddr(addr);
        return iRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

ssize_t sceNetGlueSend(int s, const void *buf, size_t len, int flags)
{
    int iRetval;

    /* 2.7.0 Netglue doc says flags "Not supported (always set to zero)     */
    /* So enforce this and print error if flags != 0                        */

    if (flags != 0)
    {
        printf("Warning: sceNetGlueSend() called with flags non zero (%d)\n",flags);
        flags = 0;
    }

    /* Call the NDK equivalent */
    iRetval = send(s, buf, len, flags);

    /* NetGlue and NDK both use >= 0 for success */
    if (iRetval >= 0)
    {
        return iRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

ssize_t sceNetGlueSendto
    (int s, const void *buf, size_t len, int flags,
     const sceNetGlueSockaddr_t *addr, sceNetGlueSocklen_t addrlen)
{
    int iRetval;

    /* *addr is an input so need to convert it before calling NDK */
    /* but it's a const, so have to clone it before converting it */
    sceNetGlueSockaddr_t copyAddr;
    memcpy(&copyAddr, addr, sizeof(copyAddr));

    /* convert *addr from Netglue to NDK format */
    snConvGlueToNdkSockaddr(&copyAddr);

    /* 2.7.0 Netglue doc says flags "Not supported (always set to zero)     */
    /* So enforce this and print error if flags != 0                        */

    if (flags != 0)
    {
        printf("Warning: sceNetGlueSendto() called with flags non zero (%d)\n",flags);
        flags = 0;
    }

    /* Call the NDK equivalent */
    iRetval = sendto(s, buf, len, flags, (struct sockaddr*)&copyAddr, addrlen);

    /* NetGlue and NDK both use >= 0 for success */
    if (iRetval >= 0)
    {
        return iRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

int sceNetGlueSetSifMBindRpcValue
    (u_int buffersize, u_int stacksize, int priority)
{
    /* According to Sony this function is deprecated and hasn't been        */
    /* implemented in any release of netglue since 2.4                      */
    /* NDK doesn't have the run-time options to support this in any case    */

    /* Print a warning but return indicating success */
    printf("Warning: sceNetGlueSetSifMBindRpcValue() not implemented\n");

    return 0;
}

int sceNetGlueSetsockopt
    (int s, int level, int optname, const void *optval,
     sceNetGlueSocklen_t optlen)
{
    /* At the moment the only option supported by NetGlue is TCP_NODELAY    */
    /* which uses level = IPPROTO_TCP.                                      */
    /* Because the values defined for IPPROTO_TCP and TCP_NODELAY are the   */
    /* same in NDK and Netglue, there is no need to do any conversion here  */

    int iRetval;

    /* Call the NDK equivalent */
    iRetval = setsockopt(s, level, optname, optval, optlen);

    /* NetGlue and NDK both use zero for success */
    if (iRetval == 0)
    {
        return iRetval;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

int sceNetGlueShutdown(int s, int how)
{
    int iCloseResult;
    int iShutdownResult;

    /* NDK and netglue both use the same *values* for the how parameter but */
    /* use different names, like this                                       */
    /* NDK values                 Netglue values                            */
    /* #define SD_RECEIVE  0      #define SHUT_RD    0                      */
    /* #define SD_SEND     1      #define SHUT_WR    1                      */
    /* #define SD_BOTH     2      #define SHUT_RDWR  2                      */

    /* Also netglue only supports SHUT_RDWR (2.7.0 lib ref) and a call to   */
    /* this also performs the function of closesocket()                     */
    
    if (how != SD_BOTH)
    {
        printf("Warning: sceNetGlueShutdown() called with how != SHUT_RDWR (%d)\n",how);
        how = SD_BOTH;
    }

    /* Call the NDK equivalent */
    iShutdownResult = shutdown(s, how);
    iCloseResult    = closesocket(s);

    /* NetGlue and NDK both use zero for success */
    if ((iShutdownResult == 0) && (iCloseResult == 0))
    {
        return 0;
    }
    
    /* Get / convert / store error detail */
    snGetConvStoreSocketError(s);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

int sceNetGlueSocket(int family, int type, int protocol)
{
    /* NDK and netglue both use the same values and names for the various  */
    /* #defines relevant to the paramters this function is passed:         */
    /*                                                                     */
    /* family may only be AF_INET, both NDK and netglue define this as 2   */
    /*                                                                     */
    /* type may be SOCK_STREAM (NDK and netglue define this as 1)          */
    /*          or SOCK_DGRAM  (NDK and netglue define this as 2)          */
    /*          or SOCK_RAW    (NDK not supported, netglue defines it as 3)*/
    /*                                                                     */
    /* Sony advise that DNAS does not use SOCK_RAW.                        */
    /*                                                                     */
    /* protocol is not supported by netglue (always set to zero), for      */
    /* NDK, protcol is always passed in to socket() as PF_INET             */

    int iRetval;

    /* Trap any calls using type SOCK_RAW (and any other unsupported type) */

    if ((type != SOCK_STREAM) && (type != SOCK_DGRAM))
    {
        printf("Error: sceNetGlueSocket() called with type = (%d)\n",type);
        snStoreErrno(NG_EINVAL);
        return -1;
    }

    /* Call the NDK equivalent */
    iRetval = socket(family, type, PF_INET);

    /* Positive value is a good socket descriptor, return it */ 
    if (iRetval > 0) 
    { 
        return iRetval; 
    }     

    /* NDK doesn't provide a general errno, only provided per socket, but  */
    /* failed to create the socket so no way of knowing why                */

    snStoreErrno(NG_UNSPECIFIED);

    /* NetGlue and NDK both use -1 for error */
    return -1;
}

int sceNetGlueThreadInit(int thread_id)
{
    int iResult;
    int iDontCare;
    int iThisThreadId;

    iThisThreadId = GetThreadId();

    /* thread_id == 0 means this thread */
    if (thread_id == 0)
    {
        thread_id = iThisThreadId;
    }

    /* NDK requires each thread that is going to use the socket API to      */
    /* register *itself* with the NDK API by calling sockAPIregthr() i.e.   */
    /* there is currently no way for thread A to register thread B          */
    /* so trap this and report error.                                       */

    /* If this restriction turns out to be a problem in practice then it    */
    /* will be necessary for NDK to be modified so that thread A can        */
    /* register thread B e.g. add new fn sockAPIregother()                  */

    /* so trap this and report error.                                       */

    if (iThisThreadId != thread_id)
    {
        printf("Error: sceNetGlueThreadInit() can only register own thread\n");
        return -1;
    }

    /* Add this to our internal list of registered threads */
    iResult = snAddToThreadInfo(thread_id);

    /* If it failed (list is full), return now with failure */
    if (iResult < 0)
    {
        return -1;
    }
       
    /* Now register with NDK */
    iResult = sockAPIregthr();

    /* If it worked ok, return now indicating success */
    if (iResult == 0)
    {
        return 0;
    }

    /* Failed to register with NDK so remove from local list */
    iDontCare = snRemoveFromThreadInfo(thread_id);

    return -1;
}

int sceNetGlueThreadTerminate(int thread_id)
{
    int iResult1;
    int iResult2;
    int iThisThreadId;

    iThisThreadId = GetThreadId();

    /* thread_id == 0 means this thread */
    if (thread_id == 0)
    {
        thread_id = iThisThreadId;
    }

    /* Remove from local list */
    iResult1 = snRemoveFromThreadInfo(thread_id);

    /* De-register with NDK */
    iResult2 = sockAPIderegother(thread_id);

    /* If both operations successful return indicating success */
    if ((iResult1 >= 0) && (iResult2 ==0))
    {
        return 0;
    }

    return -1;
}

} // end of extern "C"

