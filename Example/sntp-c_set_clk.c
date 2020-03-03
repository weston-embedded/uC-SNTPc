/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                               EXAMPLE
*
*                                             SNTP CLIENT
*
* Filename : sntp-c_set_clk.c
* Version  : V2.01.00
*********************************************************************************************************
* Note(s)  : (1) This example show how uC/CLK with uC/SNTP.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <Source/sntp-c.h>
#include  <sntp-c_cfg.h>
#include  <Source/clk.h>


/*
*********************************************************************************************************
*                                          App_SNTPc_SetClk()
*
* Description : Get the time from an SNTP server and set the uc/CLK module.
*
* Argument(s) : p_sntp_cfg   Pointer to SNTP server configuration.
*
* Return(s)   : DEF_FAIL,   Operation failed.
*               DEF_OK,     Operation is successful
*
* Caller(s)   : none.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

CPU_BOOLEAN App_SNTPc_SetClk (SNTPc_CFG *p_sntp_cfg)
{
    SNTP_TS         ntp_ts;
    SNTP_PKT        sntp_pkt;
    SNTPc_ERR       sntp_err;
    CPU_BOOLEAN     ret_val;

                                                                /* ----------- REQUEST TS FROM SNTP SERVER ------------ */
    ret_val = SNTPc_ReqRemoteTime( p_sntp_cfg,                  /* Send a SNTP request to the specified NTP server.     */
                                  &sntp_pkt,
                                  &sntp_err);
    if (ret_val == DEF_FAIL) {
        return (DEF_FAIL);
    }
                                                                /* ----- COMPUTE NTP TIMESTAMP FROM SERVER ANSWER ----- */
    ntp_ts = SNTPc_GetRemoteTime(&sntp_pkt, &sntp_err);         /* Get the local time from the received SNTP message... */
    if (sntp_err != SNTPc_ERR_NONE) {                           /* ...packet in the form of an NTP time stamp.          */
        return (DEF_FAIL);
    }
                                                                /* --------------------- SET CLK ---------------------- */
    ret_val = Clk_SetTS_NTP(ntp_ts.Sec);                        /* Set the local time using uC/CLK.                     */
    if (ret_val == DEF_FAIL) {
        return (DEF_FAIL);
    }

    return (DEF_OK);
}

