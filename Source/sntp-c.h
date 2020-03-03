/*
*********************************************************************************************************
*                                              uC/SNTPc
*                               Simple Mail Transfer Protocol (client)
*
*                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                             SNTP CLIENT
*
* Filename : sntp-c.h
* Version  : V2.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This header file is protected from multiple pre-processor inclusion through use of the
*               SNTPc present pre-processor macro definition.
*********************************************************************************************************
*/

#ifndef  SNTPc_PRESENT                                          /* See Note #1.                                         */
#define  SNTPc_PRESENT


/*
*********************************************************************************************************
*                                        SNTPc VERSION NUMBER
*
* Note(s) : (1) (a) The SNTPc module software version is denoted as follows :
*
*                       Vx.yy.zz
*
*                           where
*                                   V               denotes 'Version' label
*                                   x               denotes     major software version revision number
*                                   yy              denotes     minor software version revision number
*                                   zz              denotes sub-minor software version revision number
*
*               (b) The SNTPc software version label #define is formatted as follows :
*
*                       ver = x.yyzz * 100 * 100
*
*                           where
*                                   ver             denotes software version number scaled as an integer value
*                                   x.yyzz          denotes software version number, where the unscaled integer
*                                                       portion denotes the major version number & the unscaled
*                                                       fractional portion denotes the (concatenated) minor
*                                                       version numbers
*********************************************************************************************************
*/

#define  SNTPc_VERSION                                 20100u   /* See Note #1.                                         */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*********************************************************************************************************
*/

#ifdef   SNTPc_MODULE
#define  SNTPc_EXT
#else
#define  SNTPc_EXT  extern
#endif


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*
* Note(s) : (1) The SNTPc module files are located in the following directories :
*
*               (a) \<Your Product Application>\sntp-c_cfg.h
*
*               (b) \<Network Protocol Suite>\Source\net_*.*
*
*               (c) \<SNTPc>\Source\sntp-c.h
*                                  \sntp-c.c
*
*                       where
*                               <Your Product Application>      directory path for Your Product's Application
*                               <Network Protocol Suite>        directory path for network protocol suite
*                               <SNTPs>                         directory path for SNTPc module
*
*           (2) CPU-configuration software files are located in the following directories :
*
*               (a) \<CPU-Compiler Directory>\cpu_*.*
*               (b) \<CPU-Compiler Directory>\<cpu>\<compiler>\cpu*.*
*
*                       where
*                               <CPU-Compiler Directory>        directory path for common CPU-compiler software
*                               <cpu>                           directory name for specific processor (CPU)
*                               <compiler>                      directory name for specific compiler
*
*           (3) NO compiler-supplied standard library functions SHOULD be used.
*
*               (a) Standard library functions are implemented in the custom library module(s) :
*
*                       \<Custom Library Directory>\lib_*.*
*
*                           where
*                                   <Custom Library Directory>      directory path for custom library software
*
*           (4) Compiler MUST be configured to include as additional include path directories :
*
*               (a) '\<Your Product Application>\' directory                            See Note #1a
*
*               (b) '\<Network Protocol Suite>\'   directory                            See Note #1b
*
*               (c) '\<SNTPc>\' directory                                               See Note #1c
*
*               (d) (1) '\<CPU-Compiler Directory>\'                  directory         See Note #2a
*                   (2) '\<CPU-Compiler Directory>\<cpu>\<compiler>\' directory         See Note #2b
*
*               (e) '\<Custom Library Directory>\' directory                            See Note #3a
*********************************************************************************************************
*/

#include  <cpu.h>                                               /* CPU Configuration              (see Note #2b)        */
#include  <cpu_core.h>                                          /* CPU Core Library               (see Note #2a)        */

#include  <lib_def.h>                                           /* Standard        Defines        (see Note #3a)        */

#include  <sntp-c_cfg.h>                                        /* SNTP Client Configuration File (see Note #1a)        */

#include  <Source/net_type.h>                                   /* Network Protocol Suite         (see Note #1b)        */

#include  <Source/net_sock.h>

#include  "Source/sntp-c_type.h"


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        SNTP MESSAGE DEFINES
*
* Note(s) : (1) See 'SNTP MESSAGE DATA TYPE  Note #2' for flag fields.
*********************************************************************************************************
*/

#define  SNTPc_MSG_FLAG_SHIFT                             24

#define  SNTPc_MSG_FLAG_LI_SHIFT                           6
#define  SNTPc_MSG_FLAG_VN_SHIFT                           3


/*
*********************************************************************************************************
*                                 SNTP MESSAGE LEAP INDICATOR DEFINES
*********************************************************************************************************
*/

#define  SNTPc_MSG_LI_NO_WARNING                           0
#define  SNTPc_MSG_LI_LAST_MIN_61                          1
#define  SNTPc_MSG_LI_LAST_MIN_59                          2
#define  SNTPc_MSG_LI_ALARM_CONDITION                      3


/*
*********************************************************************************************************
*                                        SNTP MESSAGE VERSION
*********************************************************************************************************
*/

#define  SNTPc_MSG_VER_4                                   4


/*
*********************************************************************************************************
*                                          SNTP MESSAGE MODE
*********************************************************************************************************
*/

#define  SNTPc_MSG_MODE_RESERVED                           0
#define  SNTPc_MSG_MODE_SYM_ACT                            1
#define  SNTPc_MSG_MODE_SYM_PAS                            2
#define  SNTPc_MSG_MODE_CLIENT                             3
#define  SNTPc_MSG_MODE_SERVER                             4
#define  SNTPc_MSG_MODE_BROADCAST                          5
#define  SNTPc_MSG_MODE_RESERVED_NTP_CTRL_MSG              6
#define  SNTPc_MSG_MODE_RESERVED_PRIVATE                   7


/*
*********************************************************************************************************
*                                       SNTP DFLT CONFIG VALUE
*********************************************************************************************************
*/

#define  SNTPc_DFLT_MAX_RX_TIMEOUT_MS                    5000    /* Maximum inactivity time (ms) on RX.                  */
#define  SNTPc_DFLT_MAX_TX_TIMEOUT_MS                    5000    /* Maximum inactivity time (ms) on TX.                  */
#define  SNTPc_DFLT_IPPORT                                123


/*
*********************************************************************************************************
*                                     SNTPc ERROR CODES DATA TYPE
*********************************************************************************************************
*/

typedef enum {
    SNTPc_ERR_NONE,

    SNTPc_ERR_MEM_ALLOC,                                        /* Memory allocation error.                             */
    SNTPc_ERR_FAULT_INIT,                                       /* Initialization faulted.                              */
    SNTPc_ERR_NULL_PTR,                                         /* Ptr arg(s) passed NULL ptr.                          */
    SNTPc_ERR_ACQUIRE_LOCK,                                     /* Failed to acquire module lock.                       */
    SNTPc_ERR_RX,                                               /* Error occured during packet reception.               */
    SNTPc_ERR_TX,                                               /* Error occurred during request transmission.          */
    SNTPc_ERR_SERVER_CFG,                                       /* Error in the configuration of the server.            */

}SNTPc_ERR;


/*
*********************************************************************************************************
*                                    SNTP MESSAGE PACKET DATA TYPE
*
* Note(s) : (1) See RFC #2030, Section 4 for NTP message format.
*
*           (2) The first 32 bits of the NTP message (CW) contain the following fields :
*
*                   1                   2                   3
*                   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*                   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*                   |LI | VN  |Mode |    Stratum    |     Poll      |   Precision   |
*                   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*********************************************************************************************************
*/

typedef struct sntp_ts {
    CPU_INT32U  Sec;
    CPU_INT32U  Frac;
} SNTP_TS;

typedef struct sntp_pkt {
    CPU_INT32U  CW;                                             /* Ctrl word (see Note #2).                             */
    CPU_INT32U  RootDly;                                        /* Round trip dly.                                      */
    CPU_INT32U  RootDispersion;                                 /* Nominal err returned by server.                      */
    CPU_INT32U  RefID;                                          /* Server ref ID.                                       */
    SNTP_TS     TS_Ref;                                         /* Timestamp of the last sync.                          */
    SNTP_TS     TS_Originate;                                   /* Local timestamp when sending req.                    */
    SNTP_TS     TS_Rx;                                          /* Remote timestamp when receiving req.                 */
    SNTP_TS     TS_Tx;                                          /* Remote timestamp when sending res.                   */
} SNTP_PKT;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

CPU_BOOLEAN  SNTPc_Init               (const SNTPc_CFG      *p_cfg,       /* Initialize the SNTPc Module.               */
                                             SNTPc_ERR      *p_err);

CPU_BOOLEAN  SNTPc_SetDfltCfg         (const SNTPc_CFG      *p_cfg,       /* Set the Default server configuration.      */
                                             SNTPc_ERR      *p_err);

CPU_BOOLEAN  SNTPc_ReqRemoteTime      (const SNTPc_CFG      *p_cfg,       /* Request remote time from a NTP server.     */
                                             SNTP_PKT       *ppkt,
                                             SNTPc_ERR      *p_err);

SNTP_TS      SNTPc_GetRemoteTime      (      SNTP_PKT       *ppkt,        /* Get remote time (NTP timestamp).           */
                                             SNTPc_ERR      *p_err);

CPU_INT32U   SNTPc_GetRoundTripDly_us (      SNTP_PKT       *ppkt,        /* Get pkt round trip delay.                  */
                                             SNTPc_ERR      *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                          /* End of lib mem module include.                       */

