/*
//  ------------------------------------------------------------------------

File: config.h

collection of global variables used for runtime configuration.

//  ------------------------------------------------------------------------
*/

#ifndef _CONFIG_H
#define _CONFIG_H


/*  --------------------------------------------------------
 *  global variables/constants
 *  --------------------------------------------------------
 *  choose between:
 *  -> declaration (GLOBAL undefined), or
 *  -> definition  (GLOBAL defined)
 *
 *  GLOBAL should be defined in exactly one file (per project).
 */

#ifdef GLOBAL
  #define EXTERN
#else
  #define EXTERN extern
#endif


    //  select one of the following NET_DB defines.
    //  note that the host0:net.db is unencoded.
EXTERN  char *g_netdb
#ifdef  GLOBAL
        = "mc0:";
//        = "mc1:";
//        = "host0:/usr/local/sce/conf/net/net.db";
//        = "host0:../conf/good/net.db";
#endif
;


    //  specify path to SCE-supplied modules
EXTERN  char *g_modRoot	
#ifdef  GLOBAL
        = "host0:/usr/local/sce/iop/modules/"
#endif
;


EXTERN const char *kConnectionTypeDescs[]
#ifdef  GLOBAL
=
{
  "invalid",
  "Ethernet, static IP",
  "Ethernet, DHCP",
  "Ethernet, PPPoE",
  "Modem, PPP",
  "Modem, AOL PPP"
}
#endif
;


/*  --------------------------------------------------------
 *  debugging information enable/disable
 *  --------------------------------------------------------
 */

#define DEBUG


    //  use dprintf() for non-essential debug messages only.
#ifdef  DEBUG
#define dprintf(_args...) \
        printf("%s> ", __FILE__); \
        printf(_args);
#else
#define dprintf(_args...)
#endif  // DEBUG


#endif  // _CONFIG_H

