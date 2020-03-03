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
*                                   SNTP CLIENT CONFIGURATION FILE
*
*                                              TEMPLATE
*
* Filename : sntp-c_cfg.h
* Version  : V2.01.00
*********************************************************************************************************
*/

#ifndef SNTPc_CFG_MODULE_PRESENT
#define SNTPc_CFG_MODULE_PRESENT

#include  <Source/sntp-c_type.h>


/*
*********************************************************************************************************
*                                  SNTPc ARGUMENT CHECK CONFIGURATION
*
* Note(s) : (1) Configure SNTPc_CFG_ARG_CHK_EXT_EN to enable/disable the SNTP client external argument
*               check feature :
*
*               (a) When ENABLED,  ALL arguments received from any port interface provided by the developer
*                   are checked/validated.
*
*               (b) When DISABLED, NO  arguments received from any port interface provided by the developer
*                   are checked/validated.
*********************************************************************************************************
*/
                                                                /* Configure external argument check feature ...        */
                                                                /* See Note 1.                                          */
#define  SNTPc_CFG_ARG_CHK_EXT_EN                   DEF_ENABLED
                                                                /* DEF_DISABLED     External argument check DISABLED    */
                                                                /* DEF_ENABLED      External argument check ENABLED     */

/*
*********************************************************************************************************
*                                SNTPc RUN-TIME STRUCTURE CONFIGURATION
*
* Note(s) : (1) This structure should be defined into a 'C' file.
*********************************************************************************************************
*/

extern  const  SNTPc_CFG  SNTPc_Cfg;


/*
*********************************************************************************************************
*                                                TRACING
*********************************************************************************************************
*/

#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                                   0
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                                  1
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                                   2
#endif

#define  SNTPc_TRACE_LEVEL                      TRACE_LEVEL_INFO
#define  SNTPc_TRACE                            printf

#endif
