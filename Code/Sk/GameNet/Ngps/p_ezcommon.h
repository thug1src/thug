/*
//  ------------------------------------------------------------------------

File: common.h

shared headers between EE and IOP projects.

//  ------------------------------------------------------------------------
*/

#ifndef	_COMMON_H
#define	_COMMON_H


/*  --------------------------------------------------------
 *  constants
 *  --------------------------------------------------------
 */

#define kModName_ezNetCnf "eznetcnf"
#define kModName_ezNetCtl "eznetctl"

/*
 *  SIF RPC identifier (aka SIFNUM)
 *  used with sceSifRegisterRpc() and sceSifBindRpc()
 */
#define kRPCId_ezNetCnf	0x75499128
#define kRPCId_ezNetCtl	0x75488909

/*
 *  SIF RPC buffer size
 */
#define kRPCBufferSize  (256 + 64)


/*
 *  SIF DMA are performed in 16-byte blocks from IOP -> EE.
 *  enforce padding in both DMA directions.
 */
#define kDMAWait        0
#define kDMANoWait      1
#define kDMABlockSize   16
#define DMA_PAD(_size)  (((_size) + (kDMABlockSize-1)) & ~(kDMABlockSize-1))


/*  --------------------------------------------------------
 *  enums
 *  --------------------------------------------------------
 */

/*
 *  eznetcnf RPC function ids
 */
typedef enum {
  eCnfGetCount = 0,
  eCnfGetCombinationList,
  eCnfGetEnvData,

  eCnfEnd = 0x1ffff           // for enum size consistency
} ezNetCnfFunctionId;

/*
 *  eznetctl RPC function ids
 */
typedef enum {
  eCtlGetEnvDataAddress = 0,
  eCtlSetConfiguration,
  eCtlSetCombination,
  eCtlWatchStatus,
  eCtlDownInterface,

  eCtlEnd = 0x1ffff           // for enum size consistency
} ezNetCtlFunctionId;

/*
 *  eznetcnf connection types (deprecated)
 */
typedef enum {
  eConnInvalid = 0,           // invalid combination of devType and ifcType
  eConnStatic,                // Ethernet (static IP)
  eConnDHCP,                  // Ethernet (DHCP)
  eConnPPPoE,                 // Ethernet (PPP over Ethernet)
  eConnPPP,                   // modem (PPP)
  eConnAOLPPP,                // modem (AOL PPP)

  eConnPadding = 0x1ffff      // for enum size consistency
} ezConnectionType;

/*
 *  network interface protocol family
 */
typedef enum {
  eInterfacePPP,      // modem PPP
  eInterfacePPPoE,    // PPP over Ethernet
  eInterfaceARP       // Ethernet (static IP and DHCP)
} ezInterfaceType;

/*
 *  netcnf file types
 */
typedef enum {
  eCombinationFile = 0,       // net???.cnf
  eInterfaceFile,             // ifc???.cnf
  eDeviceFile,                // dev???.cnf

  eFilePadding = 0x1ffff      // for enum size consistency
} ezNetCnfType;


/*  --------------------------------------------------------
 *  typedefs
 *  --------------------------------------------------------
 */

#define kMaxCombinations  10  // maximum number of Combinations
#define kStrBufferSize    256 // maximum size for any text field
#define kStrDisplaySize   32  // display size for any text field


typedef struct ezNetCnfCombination {
  int isActive;
  ezConnectionType connectionType;
  char combinationName[kStrDisplaySize];
  char ifcName[kStrDisplaySize];
  char devName[kStrDisplaySize];
} ezNetCnfCombination_t;


typedef struct ezNetCnfCombinationList {
  int listLength;
  unsigned int defaultIndex;
  unsigned int netdbOrder[kMaxCombinations];
  ezNetCnfCombination_t list[kMaxCombinations];
} ezNetCnfCombinationList_t __attribute__((aligned(64)));


typedef struct ezNetCtlStatus {
  u_int clientAddr;
  int id;
  int event;
  int state;
  char message[kStrBufferSize];
} ezNetCtlStatus_t __attribute__((aligned(64)));


/*
 *  RPC buffer type for ezNetCnfGetCount()
 */
typedef struct {
  int result;
  ezNetCnfType fileType;
  char netdbPath[kStrBufferSize];
} rpcGetCount_t;

/*
 *  RPC buffer type for ezNetCnfGetCombinationList()
 */
typedef struct {
  int result;
  u_int clientAddr;
  char netdbPath[kStrBufferSize];
} rpcGetCombinationList_t;

/*
 *  RPC buffer type for ezNetCnfGetEnvData()
 */
typedef struct {
  int result;
  u_int clientAddr;
  char netdbPath[kStrBufferSize];
  char combinationName[kStrDisplaySize];
} rpcGetEnvData_t;

#endif	// _COMMON_H

