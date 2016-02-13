/*
//  ------------------------------------------------------------------------

File: libezcnf.c

//  ------------------------------------------------------------------------
*/

#include "p_ezmain.h"


static int m_rpcRunning = 0;
static sceSifClientData m_rpcCd;;
static int m_rpcBuf[kRPCBufferSize/4] __attribute__((aligned(64)));


#ifdef  DEBUG
#define ASSERT_RPC(_size)           \
  if (!m_rpcRunning) {              \
    printf("**** RPC services are not currently running\n"); \
    return -1;                      \
  }                                 \
  if (_size > kRPCBufferSize) {     \
    printf("**** RPC data size 0x%x exceeds RPC buffer size 0x%x\n", \
           _size, kRPCBufferSize);  \
    return -1;                      \
  }
#else
#define ASSERT_RPC(_size)
#endif	// DEBUG


/*  --------------------------------------------------------
 *  PUBLIC library initialization and finalization
 *  --------------------------------------------------------
 */

/*
 *  binds to eznetcnf.irx RPC services.
 *  r == 1  after RPC is bound.
 */
int
initEzNetCnf( void )
{
#ifdef  DEBUG
  int id = sceSifSearchModuleByName(kModName_ezNetCnf);
  if (id < 0) {
    printf("\"%s\" module not found (%d)\n", kModName_ezNetCnf, id);
  }
#endif

  // bind RPC
  while (1) {
    int i = 0x10000;
    if (sceSifBindRpc(&m_rpcCd, kRPCId_ezNetCnf, 0) < 0) {
      printf("could not bind to eznetcnf service\n");
    }
    if (m_rpcCd.serve != 0) break;
    while (i--)
      ;
  }

  return m_rpcRunning = 1;
}

/*
 *  exit and unload eznetcnf.irx, then unbind RPC services.
 *  r == 0  after RPC is unbound.
 */
int
exitEzNetCnf( void )
{
  if (m_rpcRunning == 1) {
    // call using non-blocking mode,
    // since eznetcnf unloads and the call cannot return.
    sceSifCallRpc(&m_rpcCd, eCnfEnd, SIF_RPCM_NOWAIT, 
                  0, 0, 0, 0, 0, 0);
    // not sure if this is necessary.
    sceSifInitRpc(0);
  }

  return m_rpcRunning = 0;
}


/*  --------------------------------------------------------
 *  PUBLIC library interfaces
 *  --------------------------------------------------------
 */

/*
 *  given the path to a netdb database, and a netcnf file type,
 *  return the number of files listed in the database. note
 *  sceNetCnfGetCount() returns 0 normally when no netdb file
 *  can be found, or even when there is no Memory Card.
 *
 *  r >  0  for valid netdb with at least 1 Combination,
 *  r <= 0  for error or no Combinations.
 */
int 
ezNetCnfGetCount(const char *netdb, ezNetCnfType type)
{
  rpcGetCount_t *rpcData = (rpcGetCount_t *)&m_rpcBuf;
  
  ASSERT_RPC(sizeof(rpcGetCount_t));
  
  // pack RPC data
  strcpy(rpcData->netdbPath, netdb);
  rpcData->fileType = type;
  
  // call RPC
  sceSifCallRpc(&m_rpcCd, eCnfGetCount, 
                0,
                &m_rpcBuf, sizeof(rpcGetCount_t),
                &m_rpcBuf, sizeof(rpcGetCount_t),
                0, 0);
  
  dprintf("ezNetCnfGetCount returned %d\n", rpcData->result);
  return rpcData->result;
}

/*
 *  given the path to a netdb database, retrieve a list of Combinations 
 *  to the memory address specified at addr. this list contains all the 
 *  information needed to present to the user.
 *
 *  r == 6  for valid Memory Card netdb with at least 1 Combination,
 *  r == 10 for valid PFS netdb with at least 1 Combination,
 *  r == N  for valid host netdb with 0 < N <= 10 Combinations,
 *  r <= 0  for error or no Combinations.
 */
int
ezNetCnfGetCombinationList(char *netdb, ezNetCnfCombinationList_t *addr)
{
  rpcGetCombinationList_t *rpcData = (rpcGetCombinationList_t *)&m_rpcBuf;

  ASSERT_RPC(sizeof(rpcGetCombinationList_t));
  
  // pack RPC data
  strcpy(rpcData->netdbPath, netdb);
  rpcData->clientAddr = (u_int)addr;
  
  // call RPC
  sceSifCallRpc(&m_rpcCd, eCnfGetCombinationList, 
                0,
                &m_rpcBuf, sizeof(rpcGetCombinationList_t),
                &m_rpcBuf, sizeof(rpcGetCombinationList_t),
                0, 0);
  
  dprintf("ezNetCnfGetCombinationList returned %d\n", rpcData->result);
  return rpcData->result;
}

/*
 *  given a previously generated list of combination configurations,
 *  return the Combination designated as the "default" in netdb.
 *
 *  r != NULL   for valid pList,
 *  r == NULL   for empty or invalid pList.
 */
ezNetCnfCombination_t *
ezNetCnfGetDefault(ezNetCnfCombinationList_t *pList)
{
  // check for valid parameters
  if (pList == NULL || pList->listLength == 0) {
    printf("ezNetCnfGetDefault: pList is empty\n");
    return NULL;
  }
  if (pList->defaultIndex >= (unsigned int) pList->listLength) {
    printf("ezNetCnfGetDefault: invalid defaultIndex (%d)\n",
           pList->defaultIndex);
    return NULL;
  }

  return &pList->list[pList->defaultIndex];
}

/*
 *  given the list of combinations and an ordinal number (1..N),
 *  return "CombinationN" -- REMEMBER to adjust zero-based indices.
 *
 *  r != NULL   for valid pList and valid number,
 *  r == NULL   for empty or invalid pList, or invalid number.
 */
ezNetCnfCombination_t *
ezNetCnfGetCombination(ezNetCnfCombinationList_t *pList, int number)
{
  // check for valid parameters
  if (pList == NULL || pList->listLength == 0) {
    printf("ezNetCnfGetCombination: pList is empty\n");
    return NULL;
  }
  if ((number < 1) || (number > pList->listLength)) {
    printf("ezNetCnfGetCombination: invalid Combination number (%d)\n",
           number);
    return NULL;
  }
    
  return &pList->list[number-1];
}

/*
 *  given the netdb path and a designated Combination, read and convert
 *  the data to a sceNetcnfifData record at the specified memory address.
 *  this data will contain all essential configuration properties.
 *
 *  r == 0  for successfully loading Combination,
 *  r <  0  for error.
 */
int
ezNetCnfGetEnvData(char *netdb, char *combinationName, sceNetcnfifData_t *addr)
{
  rpcGetEnvData_t *rpcData = (rpcGetEnvData_t *)&m_rpcBuf;

  ASSERT_RPC(sizeof(rpcGetEnvData_t));
  // pack the RPC request structure
  strcpy(rpcData->netdbPath, netdb);
  strcpy(rpcData->combinationName, combinationName);
  rpcData->clientAddr = (u_int)addr;

  // call RPC
  sceSifCallRpc(&m_rpcCd, eCnfGetEnvData, 
                0,
                &m_rpcBuf, sizeof(rpcGetEnvData_t),
                &m_rpcBuf, sizeof(rpcGetEnvData_t),
                0, 0);
  
  dprintf("ezNetCnfGetEnvData returned %d\n", rpcData->result);
  return rpcData->result;
}


