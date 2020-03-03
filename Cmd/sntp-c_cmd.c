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
*                                      uC/SNTPc CMD SOURCE CODE
*
* Filename : sntp-c_cmd.c
* Version  : V2.01.00
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#define  MICRIUM_SOURCE
#define  SNTPc_CMD_MODULE

#include  "../Source/sntp-c.h"
#include  "sntp-c_cmd.h"
#include  <Source/net_util.h>
#include  <Source/net_ascii.h>
#include  <Source/net_sock.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define SNTPc_CMD_HELP_1                               "\r\nusage: sntp_get [options]\r\n\r\n"
#define SNTPc_CMD_HELP_2                               " -6,           Test SNTPc using IPv6 \r\n"
#define SNTPc_CMD_HELP_3                               " -4,           Test SNTPc using IPv4 \r\n"
#define SNTPc_CMD_HELP_4                               " -d,           Test SNTPc using server domain name\r\n\r\n"

#define SNTPc_GET_MSG_STR1                             "\r\nNTP Time             : "
#define SNTPc_GET_MSG_STR2                             "\r\nRound trip delay (us): "

#define SNTPc_CMD_FAIL                                 "FAIL "

#define SNTPc_CMD_SERVER_IPV4                          "192.168.0.2"
#define SNTPc_CMD_SERVER_IPV6                          "fe80::1234:5678"
#define SNTPc_CMD_SERVER_DOMAIN_NAME                   "0.pool.ntp.org"

#define SNTPc_CMD_PARSER_DNS                           ASCII_CHAR_LATIN_LOWER_D
#define SNTPc_CMD_PARSER_IPv6                          ASCII_CHAR_DIGIT_SIX
#define SNTPc_CMD_PARSER_IPv4                          ASCII_CHAR_DIGIT_FOUR
#define SNTPc_CMD_ARG_PARSER_CMD_BEGIN                 ASCII_CHAR_HYPHEN_MINUS


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

CPU_INT16S  SNTPcCmd_Get  (CPU_INT16U        argc,
                           CPU_CHAR         *p_argv[],
                           SHELL_OUT_FNCT    out_fnct,
                           SHELL_CMD_PARAM  *p_cmd_param);

CPU_INT16S  SNTPcCmd_Help (CPU_INT16U        argc,
                           CPU_CHAR         *p_argv[],
                           SHELL_OUT_FNCT    out_fnct,
                           SHELL_CMD_PARAM  *p_cmd_param);


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/

static  SHELL_CMD SNTPc_CmdTbl[] =
{
    {"sntp_get" , SNTPcCmd_Get},
    {"sntp_help" , SNTPcCmd_Help},
    {0, 0 }
};


/*
*********************************************************************************************************
*                                           SNTPcCmd_Init()
*
* Description : Add uC/SNTPc cmd stubs to uC-Shell.
*
* Argument(s) : p_err    is a pointer to an error code which will be returned to your application:
*
*                             SNTPc_CMD_ERR_NONE              No error.
*
*                             SNTPc_CMD_ERR_SHELL_INIT        Command table not added to uC-Shell
*
* Return(s)   : none.
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  SNTPcCmd_Init (SNTPc_CMD_ERR  *p_err)
{
    SHELL_ERR  err;


    Shell_CmdTblAdd("sntp", SNTPc_CmdTbl, &err);

    if (err == SHELL_ERR_NONE) {
        *p_err = SNTPc_CMD_ERR_NONE;
    } else {
        *p_err = SNTPc_CMD_ERR_SHELL_INIT;
    }
}


/*
*********************************************************************************************************
*                                           SNTPcCmd_Get()
*
* Description : Get NTP timestamp from the server and compute the roundtrip delay.
*
* Argument(s) : argc            is a count of the arguments supplied.
*
*               p_argv          an array of pointers to the strings which are those arguments.
*
*               out_fnct        is a callback to a respond to the requester.
*
*               p_cmd_param     is a pointer to additional information to pass to the command.
*
*
* Return(s)   :
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT16S SNTPcCmd_Get(CPU_INT16U            argc,
                            CPU_CHAR         *p_argv[],
                            SHELL_OUT_FNCT    out_fnct,
                            SHELL_CMD_PARAM  *p_cmd_param)
{
    CPU_INT16S          ret_val;
    CPU_INT08U          i;
    SNTP_PKT            pkt;
    SNTP_TS             ts;
    CPU_INT32U          roundtrip;
    CPU_BOOLEAN         result;
    CPU_CHAR            str_output[32];
    CPU_INT16U          str_len;
    SNTPc_ERR           sntp_err;
    SNTPc_CFG           cfg;
    SNTPc_CFG          *p_cfg;


    cfg.ReqRxTimeout_ms = SNTPc_DFLT_MAX_RX_TIMEOUT_MS;
    cfg.ServerPortNbr   = SNTPc_DFLT_IPPORT;
    p_cfg               = DEF_NULL;
                                                                /* ----------------- PARSE ARGUMENTS ------------------ */
    if (argc != 0) {
        for (i = 1; i < argc; i++) {
            if (*p_argv[i] == SNTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                switch (*(p_argv[i] + 1)) {
                    case SNTPc_CMD_PARSER_IPv6:
                         cfg.ServerHostnamePtr = SNTPc_CMD_SERVER_IPV6;
                         cfg.ServerAddrFamily  = NET_IP_ADDR_FAMILY_IPv6;
                         p_cfg                 = &cfg;
                         if (argc != i + 1) {
                             if (*p_argv[i+1] != SNTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                 cfg.ServerHostnamePtr = p_argv[i+1];
                                 i++;
                             }
                         }
                         break;

                    case SNTPc_CMD_PARSER_IPv4:
                         cfg.ServerHostnamePtr = SNTPc_CMD_SERVER_IPV4;
                         cfg.ServerAddrFamily  = NET_IP_ADDR_FAMILY_IPv4;
                         p_cfg                 = &cfg;
                         if (argc != i + 1) {
                             if (*p_argv[i+1] != SNTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                 cfg.ServerHostnamePtr = p_argv[i+1];
                                 i++;
                             }
                         }
                         break;

                    case SNTPc_CMD_PARSER_DNS:
                         cfg.ServerHostnamePtr = SNTPc_CMD_SERVER_DOMAIN_NAME;
                         cfg.ServerAddrFamily  = NET_IP_ADDR_FAMILY_NONE;
                         p_cfg                 = &cfg;
                         if (argc != i + 1) {
                             if (*p_argv[i+1] != SNTPc_CMD_ARG_PARSER_CMD_BEGIN) {
                                 cfg.ServerHostnamePtr = p_argv[i+1];
                                 i++;
                             }
                         }
                         break;

                    default:
                         goto exit_fail;
                         break;
                }
            }
        }
    } else {
        goto exit_fail;
    }
                                                                /* ------------- REQ NTP TIME FROM SERVER ------------- */
    result = SNTPc_ReqRemoteTime(p_cfg,&pkt,&sntp_err);
    if (result == DEF_FAIL) {
        goto exit_fail;
    }
    ts         = SNTPc_GetRemoteTime(&pkt,&sntp_err);
    roundtrip  = SNTPc_GetRoundTripDly_us(&pkt,&sntp_err);

                                                                /* ------------------ PRINT RESULTS ------------------- */
    str_len = Str_Len(SNTPc_GET_MSG_STR1);                      /* NTP Time.                                            */
    out_fnct(SNTPc_GET_MSG_STR1,
             str_len,
             p_cmd_param->pout_opt);
    Str_FmtNbr_Int32U(ts.Sec,
                      DEF_INT_32U_NBR_DIG_MAX,
                      DEF_NBR_BASE_DEC,
                      DEF_NULL,
                      DEF_NO,
                      DEF_YES,
                      str_output);
    str_len = Str_Len(str_output);
    out_fnct(str_output,
             str_len,
             p_cmd_param->pout_opt);
                                                                /* Roundtrip delay.                                     */
    str_len = Str_Len(SNTPc_GET_MSG_STR2);
    out_fnct(SNTPc_GET_MSG_STR2,
             str_len,
             p_cmd_param->pout_opt);
    Str_FmtNbr_Int32U(roundtrip,
                      DEF_INT_32U_NBR_DIG_MAX,
                      DEF_NBR_BASE_DEC,
                      DEF_NULL,
                      DEF_NO,
                      DEF_YES,
                      str_output);
    str_len = Str_Len(str_output);
    out_fnct(str_output,
             str_len,
             p_cmd_param->pout_opt);

    out_fnct(STR_NEW_LINE,
             STR_NEW_LINE_LEN,
             p_cmd_param->pout_opt);

    out_fnct(STR_NEW_LINE,
             STR_NEW_LINE_LEN,
             p_cmd_param->pout_opt);

    ret_val = str_len;

    return (ret_val);

exit_fail:
    out_fnct(SNTPc_CMD_FAIL,
             sizeof(SNTPc_CMD_FAIL),
             p_cmd_param->pout_opt);
    out_fnct(STR_NEW_LINE,
             STR_NEW_LINE_LEN,
             p_cmd_param->pout_opt);
    return (1);
}


/*
*********************************************************************************************************
*                                           SNTPc_Cmd_Help()
*
* Description : Print SNTPc command help.
*
* Argument(s) : argc            is a count of the arguments supplied.
*
*               p_argv          an array of pointers to the strings which are those arguments.
*
*               out_fnct        is a callback to a respond to the requester.
*
*               p_cmd_param     is a pointer to additional information to pass to the command.
*
*
* Return(s)   : The number of positive data octets transmitted, if NO errors
*
*               SHELL_OUT_RTN_CODE_CONN_CLOSED,                 if implemented connection closed
*
*               SHELL_OUT_ERR,                                  otherwise
*
* Caller(s)   : AppTaskStart().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT16S SNTPcCmd_Help (CPU_INT16U        argc,
                          CPU_CHAR         *p_argv[],
                          SHELL_OUT_FNCT    out_fnct,
                          SHELL_CMD_PARAM  *p_cmd_param)
{
    CPU_INT16U  cmd_namd_len;
    CPU_INT16S  output;
    CPU_INT16S  ret_val;


    cmd_namd_len = Str_Len(SNTPc_CMD_HELP_1);
    output       = out_fnct(SNTPc_CMD_HELP_1,
                            cmd_namd_len,
                            p_cmd_param->pout_opt);

    cmd_namd_len = Str_Len(SNTPc_CMD_HELP_2);
    output       = out_fnct(SNTPc_CMD_HELP_2,
                            cmd_namd_len,
                            p_cmd_param->pout_opt);

    cmd_namd_len = Str_Len(SNTPc_CMD_HELP_3);
    output       = out_fnct(SNTPc_CMD_HELP_3,
                            cmd_namd_len,
                            p_cmd_param->pout_opt);

    cmd_namd_len = Str_Len(SNTPc_CMD_HELP_4);
    output       = out_fnct(SNTPc_CMD_HELP_4,
                            cmd_namd_len,
                            p_cmd_param->pout_opt);


    switch (output) {
        case SHELL_OUT_RTN_CODE_CONN_CLOSED:
        case SHELL_OUT_ERR:
             ret_val = SHELL_EXEC_ERR;
             break;
        default:
             ret_val = output;
    }

    return (ret_val);
}

