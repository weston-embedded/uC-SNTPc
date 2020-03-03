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
* Filename : sntp-c_cfg.c
* Version  : V2.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <lib_def.h>
#include  <Source/sntp-c.h>


/*
*********************************************************************************************************
*                                 SNTPc CLIENT CONFIGURATION STRUCTURE
*********************************************************************************************************
*/

const  SNTPc_CFG  SNTPc_Cfg = {

/*
*--------------------------------------------------------------------------------------------------------
*                                    SNTP SERVER CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/
                                                        /* NTP Server hostname or IP address.                           */
        "0.pool.ntp.org",
                                                        /* NTP Server port number.                                      */
         SNTPc_DFLT_IPPORT,
                                                        /* Select IP family of address when DNS resolution is used:     */
         NET_IP_ADDR_FAMILY_NONE,
                                                        /* NET_IP_ADDR_FAMILY_NONE: No preference between IPv6 and IPv4.*/
                                                        /* NET_IP_ADDR_FAMILY_IPv4: Only IPv4 addr will be returned.    */
                                                        /* NET_IP_ADDR_FAMILY_IPv6: Only IPv6 addr will be returned.    */

/*
*--------------------------------------------------------------------------------------------------------
*                                     TIMEOUT CONFIGURATION
*--------------------------------------------------------------------------------------------------------
*/
         SNTPc_DFLT_MAX_RX_TIMEOUT_MS,                  /* Maximum inactivity time (ms) on RX.                          */

};

