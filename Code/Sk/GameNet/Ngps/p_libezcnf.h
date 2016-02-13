/*  
//  ------------------------------------------------------------------------

File: libezcnf.h

headers for libezcnf.c

//  ------------------------------------------------------------------------
*/

#ifndef _LIBEZCNF_H
#define _LIBEZCNF_H

int initEzNetCnf(void);
int exitEzNetCnf(void);

int ezNetCnfGetCount(const char *, ezNetCnfType);
int ezNetCnfGetCombinationList(char *, ezNetCnfCombinationList_t *);
ezNetCnfCombination_t *ezNetCnfGetCombination(ezNetCnfCombinationList_t *, int);
ezNetCnfCombination_t *ezNetCnfGetDefault(ezNetCnfCombinationList_t *);
int ezNetCnfGetEnvData(char *, char *, sceNetcnfifData_t *);

#endif	// _LIBEZCNF_H

