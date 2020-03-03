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
* Filename : sntp-c.c
* Version  : V2.01.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    SNTPc_MODULE
#include  "sntp-c.h"
#include  <Source/net_sock.h>
#include  <Source/net_ascii.h>
#include  <Source/net_app.h>
#include  <Source/net_util.h>
#include  <KAL/kal.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define SNTP_TS_SEC_FRAC_SIZE       4294967296u                   /* Equivalent to 2^32.                                */

#define SNTP_MS_NBR_PER_SEC         1000u                         /* Nb of ms in a second.                              */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static SNTPc_CFG          *SNTPc_DfltCfgPtr;

static NET_IP_ADDR_FAMILY  SNTPc_DfltServerAddrFamily;

static KAL_LOCK_HANDLE     SNTPc_Lock;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  CPU_BOOLEAN  SNTPc_Rx           (NET_SOCK_ID     sock,
                                         SNTP_PKT       *ppkt,
                                         SNTPc_ERR      *p_err);

static  CPU_BOOLEAN  SNTPc_Tx           (NET_SOCK_ID     sock,
                                         NET_SOCK_ADDR  *paddr,
                                         SNTPc_ERR      *p_err);

static  void         SNTPc_AcquireLock  (SNTPc_ERR      *p_err);

static  void         SNTPc_ReleaseLock  (void);


/*
*********************************************************************************************************
*                                             SNTPc_Init()
*
* Description : Initialize the SNTPc Module.
*
* Argument(s) : p_cfg    Pointer to the default configuration use by the SNTP client.
*
*               p_err    Pointer to variable that will receive the return error code from this function :
*
*                               SNTPc_ERR_NONE           Server address successfully set.
*                               SNTPc_ERR_NULL_PTR       Invalid pointer.
*                               SNTPc_ERR_MEM_ALLOC      Memory allocation failed.
*                               SNTPc_ERR_FAULT_INIT     Initialization error.
*                               SNTPc_ERR_ACQUIRE_LOCK   Error occur while trying to acquire the module lock.
*
* Return(s)   : DEF_TRUE,  if the SNTP client initialization is successful.
*
*               DEF_FALSE, otherwise.
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

CPU_BOOLEAN  SNTPc_Init (const SNTPc_CFG   *p_cfg,
                               SNTPc_ERR   *p_err)
{
    KAL_ERR         err_kal;
    CPU_BOOLEAN     result;


#if (SNTPc_CFG_ARG_CHK_EXT_EN  == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (p_cfg == DEF_NULL) {
       *p_err  = SNTPc_ERR_NULL_PTR;
        result = DEF_FAIL;
        goto exit;
    }
#endif
                                                                /* Create the module's lock.                            */
    SNTPc_Lock = KAL_LockCreate("SNTPc Lock",
                                DEF_NULL,
                               &err_kal);
    switch (err_kal) {
        case KAL_ERR_NONE:
             break;

        case KAL_ERR_MEM_ALLOC:
            *p_err = SNTPc_ERR_MEM_ALLOC;
             result = DEF_FAIL;
             goto exit;

        default:
            *p_err = SNTPc_ERR_FAULT_INIT;
             result = DEF_FAIL;
             goto exit;
    }
                                                                /* Set the default server configuration.                */
    result = SNTPc_SetDfltCfg (p_cfg, p_err);

exit:
    return (result);
}


/*
*********************************************************************************************************
*                                          SNTPc_SetDfltCfg()
*
* Description : Set the default server configuration.
*
* Argument(s) : p_cfg   Pointer to the default configuration use by the SNTP client.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                               SNTPc_ERR_NONE           Server address successfully set.
*                               SNTPc_ERR_NULL_PTR       Invalid pointer.
*                               SNTPc_ERR_ACQUIRE_LOCK   Error occur while trying to acquire the module lock.
*
* Return(s)   : DEF_TRUE,  if the new default server configuration is successfully set.
*
*               DEF_FALSE, otherwise.
*
*
* Caller(s)   : SNTPc_Init().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

CPU_BOOLEAN  SNTPc_SetDfltCfg (const SNTPc_CFG   *p_cfg,
                                     SNTPc_ERR   *p_err)
{
    CPU_BOOLEAN     result;


#if (SNTPc_CFG_ARG_CHK_EXT_EN  == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (p_cfg == DEF_NULL) {
       *p_err = SNTPc_ERR_NULL_PTR;
        return (DEF_FAIL);
    }
#endif
                                                                /* Get SNTPc Lock.                                      */
    SNTPc_AcquireLock(p_err);
    if (*p_err != SNTPc_ERR_NONE) {
        result = DEF_FAIL;
        goto exit;
    }
                                                                /* Set default configuration                            */
    SNTPc_DfltCfgPtr = (SNTPc_CFG *)p_cfg;

    SNTPc_DfltServerAddrFamily = p_cfg->ServerAddrFamily;
                                                                /* Release SNTPc Lock.                                  */
    SNTPc_ReleaseLock();

    result = DEF_OK;

exit:
    return(result);
}


/*
*********************************************************************************************************
*                                         SNTPc_ReqRemoteTime()
*
* Description : Send a request to an NTP server and receive an SNTP packet to compute.
*
* Argument(s) : p_cfg   Pointer to the server configuration to use by the SNTP client.
*                           If DEF_NULL,    use default configuration set in the initialization.
*                           Otherwise,      use the passed configuration.
*
*               ppkt    Pointer to a SNTP_PKT variable that will contain the received SNTP packet.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                               SNTPc_ERR_NONE           The SNTP Request has been successfully processed.
*                               SNTPc_ERR_NULL_PTR       Invalid pointer.
*                               SNTPc_ERR_ACQUIRE_LOCK   Error occurred while trying to acquire the module lock.
*                               SNTPc_ERR_SERVER_CFG     Error in the Server configuration that cause the request to fail.
*                               SNTPc_ERR_TX             Error occurred during the request transmission.
*                               SNTPc_ERR_RX             Error occurred during the packet reception.
*
* Return(s)   : DEF_TRUE,  if the SNTP request has been successfully completed.
*
*               DEF_FALSE, otherwise.
*
* Caller(s)   : App_SNTPc_SetClk(),
*               SNTPcCmd_Get().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

CPU_BOOLEAN  SNTPc_ReqRemoteTime (const SNTPc_CFG     *p_cfg,
                                        SNTP_PKT      *ppkt,
                                        SNTPc_ERR     *p_err)
{
    const SNTPc_CFG               *p_server_cfg;
          NET_SOCK_ID              sock;
          NET_ERR                  err;
          NET_IP_ADDR_FAMILY       ip_family;
          NET_SOCK_ADDR            sock_addr;
          NET_TS_MS                timestamp;
          CPU_INT32U               second;
          CPU_INT32U               frac;
          CPU_FP64                 frac_float;
          CPU_BOOLEAN              is_hostname;
          CPU_BOOLEAN              is_completed;
          CPU_BOOLEAN              is_retry_allowed;
          CPU_BOOLEAN              result;


#if (SNTPc_CFG_ARG_CHK_EXT_EN  == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (ppkt == DEF_NULL) {
       *p_err = SNTPc_ERR_NULL_PTR;
        result = DEF_FAIL;
        goto exit;
    }
#endif
                                                                /* ------------- ACQUIRE SNTP MODULE LOCK ------------- */
    SNTPc_AcquireLock(p_err);
    if (*p_err != SNTPc_ERR_NONE) {
        result = DEF_FAIL;
        goto exit;
    }
                                                                /* --------------- SELECT SERVER CONFIG --------------- */
    if (p_cfg == DEF_NULL) {
        p_server_cfg = SNTPc_DfltCfgPtr;                        /* If DEF_NULL, Use the Default Server config.          */
        ip_family    = SNTPc_DfltServerAddrFamily;
    } else {
        p_server_cfg = p_cfg;                                   /* Otherwise, use the passed configuration.             */
        ip_family    = p_server_cfg->ServerAddrFamily;
    }

                                                                /* ----------------- SELECT IP FAMILY ----------------- */
    if (ip_family == NET_IP_ADDR_FAMILY_NONE) {
        ip_family = NET_IP_ADDR_FAMILY_IPv6;                    /* If the ip family is unknown, Try first with IPV6.    */
    }

    is_completed = DEF_NO;

    while (is_completed == DEF_NO) {
                                                                /* ------------- RESOLVE SERVER HOST NAME ------------- */
        (void)NetApp_ClientDatagramOpenByHostname(&sock,
                                                   p_server_cfg->ServerHostnamePtr,
                                                   p_server_cfg->ServerPortNbr,
                                                   ip_family,
                                                  &sock_addr,
                                                  &is_hostname,
                                                  &err);
                                                                /* Check if a retry in IPv4 is allowed in case of error. */
        if ((is_hostname                    == DEF_YES                 ) &&
            (p_server_cfg->ServerAddrFamily == NET_IP_ADDR_FAMILY_NONE ) &&
            (ip_family                      == NET_IP_ADDR_FAMILY_IPv6 )) {
            is_retry_allowed = DEF_YES;
        } else {
            is_retry_allowed = DEF_NO;
        }
        if (err != NET_APP_ERR_NONE) {
            if (is_retry_allowed == DEF_YES) {
                ip_family = NET_IP_ADDR_FAMILY_IPv4;
                continue;
            }
           *p_err = SNTPc_ERR_SERVER_CFG;
            result = DEF_FAIL;
            goto exit_release;
        }

                                                                /* ----------- SET SOCKET IN BLOCKING MODE ------------ */
        NetSock_CfgBlock(sock, NET_SOCK_BLOCK_SEL_BLOCK, &err);
        if (err != NET_SOCK_ERR_NONE) {
           *p_err  = SNTPc_ERR_SERVER_CFG;
            result = DEF_FAIL;
            goto exit;
        }
                                                                /* ---------------------- TX REQ ---------------------- */
        result = SNTPc_Tx(sock, &sock_addr, p_err);             /* Send the SNTP request to the NTP server.             */
        if (*p_err != SNTPc_ERR_NONE) {
            NetSock_Close(sock, &err);
            if (is_retry_allowed == DEF_YES) {
                ip_family = NET_IP_ADDR_FAMILY_IPv4;
                continue;
            }
            goto exit_release;
        }

                                                                /* ---------------------- RX REP ---------------------- */
        NetSock_CfgTimeoutRxQ_Set( sock,                        /* Set the Rx timeout timer.                            */
                                   p_server_cfg->ReqRxTimeout_ms,
                                  &err);
        if (err != NET_SOCK_ERR_NONE) {
            if (is_retry_allowed == DEF_YES) {
                NetSock_Close(sock, &err);
                ip_family = NET_IP_ADDR_FAMILY_IPv4;
                continue;
            }
           *p_err = SNTPc_ERR_SERVER_CFG;
            goto exit_release;
        }

        result = SNTPc_Rx(sock, ppkt, p_err);                   /* Pend and Receive the SNTP packet.                    */
        if (*p_err != SNTPc_ERR_NONE) {
            NetSock_Close(sock, &err);
            if (is_retry_allowed == DEF_YES) {
                ip_family = NET_IP_ADDR_FAMILY_IPv4;
                continue;
            }
            goto exit_release;
        }
                                                                /* ----------------- COMPUTE REF TIME ----------------- */

        timestamp         = NetUtil_TS_Get_ms();
        second            = (CPU_INT32U)(timestamp / SNTP_MS_NBR_PER_SEC );
        frac_float        = ((CPU_FP64) (timestamp % SNTP_MS_NBR_PER_SEC)) / SNTP_MS_NBR_PER_SEC;
        frac              = (CPU_INT32U)(frac_float * SNTP_TS_SEC_FRAC_SIZE);
        ppkt->TS_Ref.Sec  = NET_UTIL_HOST_TO_NET_32(second);
        ppkt->TS_Ref.Frac = NET_UTIL_HOST_TO_NET_32(frac);

        NetSock_Close(sock, &err);
                                                                /* In case it's successful and the ip family was not... */
                                                                /* ...specified and it was the default config, save ... */
                                                                /* ...the ip family that worked in the default config.  */
        if (p_cfg == DEF_NULL) {
            if (SNTPc_DfltServerAddrFamily == NET_IP_ADDR_FAMILY_NONE ) {
                SNTPc_DfltServerAddrFamily = ip_family;
            }
        }
        is_completed = DEF_YES;
    }
                                                                /* ------------- RELEASE SNTP MODULE LOCK ------------- */
exit_release:
    SNTPc_ReleaseLock();

exit:
    return (result);
}


/*
*********************************************************************************************************
*                                     SNTPc_GetRemoteTime()
*
* Description : Get remote time (NTP timestamp) from a received NTP packet.
*
* Argument(s) : ppkt     Pointer to received SNTP message packet.
*
*               p_err    Pointer to variable that will receive the return error code from this function :
*
*                               SNTPc_ERR_NONE           The time has been successfully computed from the SNTP packet.
*                               SNTPc_ERR_NULL_PTR       Invalid pointer.
*
* Return(s)   : NTP timestamp.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

SNTP_TS  SNTPc_GetRemoteTime (SNTP_PKT  *ppkt,
                              SNTPc_ERR *p_err)
{
    CPU_FP64  ts_originate;
    CPU_FP64  ts_rx;
    CPU_FP64  ts_tx;
    CPU_FP64  ts_terminate;
    CPU_FP64  local_time_offset;
    CPU_FP64  local_time_float;
    SNTP_TS   local_time;


    local_time.Sec = 0u;
    local_time.Frac= 0u;

#if (SNTPc_CFG_ARG_CHK_EXT_EN  == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (ppkt == DEF_NULL) {
       *p_err = SNTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

                                                                /* ------------- GET TIME VALUES FROM PKT ------------- */
    ts_originate = (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Originate.Sec)  +
                   (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Originate.Frac) /
                   (CPU_FP64)SNTP_TS_SEC_FRAC_SIZE;

    ts_rx        = (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Rx.Sec)         +
                   (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Rx.Frac)        /
                   (CPU_FP64)SNTP_TS_SEC_FRAC_SIZE;

    ts_tx        = (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Tx.Sec)         +
                   (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Tx.Frac)        /
                   (CPU_FP64)SNTP_TS_SEC_FRAC_SIZE;

    ts_terminate = (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Ref.Sec)        +
                   (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Ref.Frac)       /
                   (CPU_FP64)SNTP_TS_SEC_FRAC_SIZE;

                                                                /* ------------------ CALCULATE TIME ------------------ */
    local_time_offset = ((ts_rx - ts_originate) +               /* Calculate local time offset from the server.         */
                         (ts_tx - ts_terminate)) / 2;

                                                                /* Apply offset to local time.                          */
    local_time_float  = (((CPU_FP64)NetUtil_TS_Get_ms()) / SNTP_MS_NBR_PER_SEC) + local_time_offset;
    local_time.Sec    = (CPU_INT32U)local_time_float;
    local_time.Frac   = (CPU_INT32U)((local_time_float - local_time.Sec) * SNTP_TS_SEC_FRAC_SIZE);

    *p_err = SNTPc_ERR_NONE;
#if (SNTPc_CFG_ARG_CHK_EXT_EN  == DEF_ENABLED)
exit:
#endif
    return (local_time);
}


/*
*********************************************************************************************************
*                                      SNTPc_GetRoundTripDly_us()
*
* Description : Get SNTP packet round trip delay from a received SNTP message packet.
*
* Argument(s) : ppkt     Pointer to received SNTP message packet.
*
*               p_err    Pointer to variable that will receive the return error code from this function :
*
*                               SNTPc_ERR_NONE           The round trip delay has been successfully computed from the SNTP packet.
*                               SNTPc_ERR_NULL_PTR       Invalid pointer.
*
* Return(s)   : SNTP packet round trip delay in us.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) If the round trip delay is faster than the precision of the system clock, then the
*                   round trip delay is approximated to 0.
*
*               (2) Only the integer part of the round trip delay is returned.
*********************************************************************************************************
*/

CPU_INT32U  SNTPc_GetRoundTripDly_us (SNTP_PKT   *ppkt,
                                      SNTPc_ERR  *p_err)
{
    CPU_FP64    ts_originate;
    CPU_FP64    ts_rx;
    CPU_FP64    ts_tx;
    CPU_FP64    ts_terminate;
    CPU_FP64    round_trip_dly;
    CPU_INT32U  round_trip_dly_32;


    round_trip_dly_32 = 0u;

#if (SNTPc_CFG_ARG_CHK_EXT_EN  == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (ppkt == DEF_NULL) {
       *p_err = SNTPc_ERR_NULL_PTR;
        goto exit;
    }
#endif

                                                                /* ------------- GET TIME VALUES FROM PKT ------------- */
    ts_originate = (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Originate.Sec)   +
                   (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Originate.Frac) /
                   (CPU_FP64)SNTP_TS_SEC_FRAC_SIZE;

    ts_rx        = (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Rx.Sec)          +
                   (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Rx.Frac)        /
                   (CPU_FP64)SNTP_TS_SEC_FRAC_SIZE;

    ts_tx        = (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Tx.Sec)          +
                   (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Tx.Frac)        /
                   (CPU_FP64)SNTP_TS_SEC_FRAC_SIZE;

    ts_terminate = (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Ref.Sec)         +
                   (CPU_FP64)NET_UTIL_NET_TO_HOST_32(ppkt->TS_Ref.Frac)       /
                   (CPU_FP64)SNTP_TS_SEC_FRAC_SIZE;

                                                                /* ------------- CALCULATE ROUND TRIP DLY ------------- */
    round_trip_dly    = ((ts_terminate - ts_originate) -
                         (ts_tx - ts_rx)) * 1000000;

    round_trip_dly_32 = (CPU_INT32U)round_trip_dly;             /* See Note #2.                                         */

    *p_err = SNTPc_ERR_NONE;

#if (SNTPc_CFG_ARG_CHK_EXT_EN  == DEF_ENABLED)
exit:
#endif
    return (round_trip_dly_32);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                              SNTPc_Rx()
*
* Description : Receive a NTP packet from server.
*
* Argument(s) : sock        Socket to receive NTP     message packet from.
*
*               ppkt        Pointer to allocated SNTP message packet.
*
*               p_err    Pointer to variable that will receive the return error code from this function :
*
*                               SNTPc_ERR_NONE           The round trip delay has been successfully computed from the SNTP packet.
*                               SNTPc_ERR_NULL_PTR       Invalid pointer.
*                               SNTPc_ERR_RX             Error during the SNTP packet reception.
*
* Return(s)   : DEF_TRUE,  if packet successfully received.
*
*               DEF_FALSE, otherwise.
*
* Caller(s)   : SNTPc_ReqRemoteTime().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  SNTPc_Rx (NET_SOCK_ID   sock,
                               SNTP_PKT     *ppkt,
                               SNTPc_ERR    *p_err)
{
    NET_SOCK_ADDR       remote_addr;
    NET_SOCK_ADDR_LEN   remote_addr_size;
    NET_SOCK_RTN_CODE   res;
    NET_ERR             err;
    CPU_BOOLEAN         result;


#if (SNTPc_CFG_ARG_CHK_EXT_EN  == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (ppkt == DEF_NULL) {
       *p_err  = SNTPc_ERR_NULL_PTR;
        result = DEF_FAIL;
        goto exit;
    }
#endif


    remote_addr_size = sizeof(remote_addr);

                                                                /* ---------------------- RX PKT ---------------------- */
    res = NetSock_RxDataFrom(                      sock,
                             (void              *) ppkt,
                             (CPU_INT16U         ) sizeof(SNTP_PKT),
                             (NET_SOCK_API_FLAGS ) NET_SOCK_FLAG_NONE,
                                                  &remote_addr,
                                                  &remote_addr_size,
                             (void              *) DEF_NULL,
                                                   0u,
                                                   DEF_NULL,
                                                  &err);

    if (res <= 0) {
       *p_err = SNTPc_ERR_RX;
        result = DEF_FAIL;
    } else {
        result = DEF_OK;
    }

#if (SNTPc_CFG_ARG_CHK_EXT_EN  == DEF_ENABLED)
exit:
#endif
    return (result);
}


/*
*********************************************************************************************************
*                                              SNTPc_Tx()
*
* Description : Send NTP packet to server.
*
* Argument(s) : sock    Socket to sent NTP message packet to.
*
*               paddr   Pointer to SNTP server sockaddr_in.
*
*
* Return(s)   : DEF_TRUE,  if packet successfully sent.
*
*               DEF_FALSE, otherwise.
*
* Caller(s)   : SNTPc_ReqRemoteTime().
*
* Note(s)     : (1) RFC # 2030, Section 5 'SNTP Client Operations' states that "[For client operations],
*                   all of the NTP header fields [...] can be set to 0, except the first octet and
*                   (optional) Transmit Timestamp fields.  In the first octet, the LI field is set to 0
*                   (no warning) and the Mode field is set to 3 (client).  The VN field must agree with
*                   the version number of the NTP/SNTP server".
*********************************************************************************************************
*/

static  CPU_BOOLEAN  SNTPc_Tx (NET_SOCK_ID     sock,
                               NET_SOCK_ADDR  *paddr,
                               SNTPc_ERR      *p_err)
{
    CPU_INT32U         cw;
    SNTP_PKT           pkt;
    CPU_INT08U         li;
    CPU_INT08U         vn;
    CPU_INT08U         mode;
    NET_SOCK_RTN_CODE  res;
    NET_ERR            err;
    NET_TS_MS          timestamp;
    CPU_INT32U         second;
    CPU_INT32U         frac;
    CPU_FP64           frac_float;
    CPU_BOOLEAN        result;


#if (SNTPc_CFG_ARG_CHK_EXT_EN  == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (paddr == DEF_NULL) {
       *p_err  = SNTPc_ERR_NULL_PTR;
        result = DEF_FAIL;
        goto exit;
    }
#endif

    Mem_Clr(&pkt, sizeof(pkt));                                 /* Clr SNTP msg pkt.                                    */


                                                                /* --------------------- INIT MSG --------------------- */
                                                                /* See Note #1.                                         */
    li       = SNTPc_MSG_LI_NO_WARNING;                         /* Set flags.                                           */
    li     <<= SNTPc_MSG_FLAG_LI_SHIFT;

    vn       = SNTPc_MSG_VER_4;
    vn     <<= SNTPc_MSG_FLAG_VN_SHIFT;

    mode     = SNTPc_MSG_MODE_CLIENT;

    cw       = li | vn | mode;
    cw     <<= SNTPc_MSG_FLAG_SHIFT;

    pkt.CW   = NET_UTIL_HOST_TO_NET_32(cw);

                                                                /* Set tx timestamp.                                    */
    timestamp         = NetUtil_TS_Get_ms();
    second            = (CPU_INT32U)(timestamp / SNTP_MS_NBR_PER_SEC );
    frac_float        = ((CPU_FP64) (timestamp % SNTP_MS_NBR_PER_SEC)) / SNTP_MS_NBR_PER_SEC ;
    frac              = (CPU_INT32U)(frac_float * SNTP_TS_SEC_FRAC_SIZE);
    pkt.TS_Tx.Sec     = NET_UTIL_HOST_TO_NET_32(second);
    pkt.TS_Tx.Frac    = NET_UTIL_HOST_TO_NET_32(frac);

                                                                /* ---------------------- TX PKT ---------------------- */
    res = NetSock_TxDataTo(sock,
                          &pkt,
                           sizeof(SNTP_PKT),
                           NET_SOCK_FLAG_SOCK_NONE,
                           paddr,
                           sizeof(NET_SOCK_ADDR),
                          &err);

    if (res <= 0) {
       *p_err  = SNTPc_ERR_TX;
        result = DEF_FAIL;
    } else {
       *p_err  = SNTPc_ERR_NONE;
        result = DEF_OK;
    }

#if (SNTPc_CFG_ARG_CHK_EXT_EN  == DEF_ENABLED)
exit:
#endif
    return (result);
}


/*
*********************************************************************************************************
*                                          SNTPc_AcquireLock()
*
* Description : Acquire the module lock.
*
* Argument(s) : p_err    Pointer to variable that will receive the return error code from this function :
*
*                               SNTPc_ERR_NONE           Server address successfully set.
*                               SNTPc_ERR_NULL_PTR       Invalid pointer.
*                               SNTPc_ERR_ACQUIRE_LOCK   Error occur while trying to acquire the module lock.
*
* Return(s)   : none.
*
* Caller(s)   : SNTPc_ReqRemoteTime(),
*               SNTPc_SetDfltCfg().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static void SNTPc_AcquireLock (SNTPc_ERR   *p_err)
{
    KAL_ERR  err_kal;


#if (SNTPc_CFG_ARG_CHK_EXT_EN  == DEF_ENABLED)
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(DEF_NULL);
    }
#endif

    KAL_LockAcquire(SNTPc_Lock, KAL_OPT_PEND_NONE, KAL_TIMEOUT_INFINITE, &err_kal);
    if(err_kal == KAL_ERR_NONE) {
        *p_err = SNTPc_ERR_NONE;
    } else {
        *p_err = SNTPc_ERR_ACQUIRE_LOCK;
    }
}


/*
*********************************************************************************************************
*                                          SNTPc_ReleaseLock()
*
* Description : Release the module lock.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : SNTPc_ReqRemoteTime(),
*               SNTPc_SetDfltCfg().
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static void SNTPc_ReleaseLock (void)
{
    KAL_ERR  err_kal;


    KAL_LockRelease(SNTPc_Lock, &err_kal);

    (void)&err_kal;
}



