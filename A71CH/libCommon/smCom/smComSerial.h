/**
 * @file smComSocket.h
 * @author NXP Semiconductors
 * @version 1.1
 * @par License
 * Copyright(C) NXP Semiconductors, 2016,2017
 * This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms.  By expressly
 * accepting such terms or by downloading, installing, activating and/or
 * otherwise using the software, you are agreeing that you have read, and
 * that you agree to comply with and are bound by, such license terms.  If
 * you do not agree to be bound by the applicable license terms, then you
 * may not retain, install, activate or otherwise use the software.
 *
 * @par Description
 *
 *****************************************************************************/

#ifndef _SCVCOMSOCKET_H_
#define _SCVCOMSOCKET_H_

#include "smCom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <errno.h>
#include <termios.h>
 

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "smCom.h"
#include "sm_printf.h"

#include <libusb-1.0/libusb.h>

#ifdef __cplusplus
extern "C" {
#endif


/* You may want to change the VENDOR_ID and PRODUCT_ID
 * depending on your device.
 */
#define VENDOR_ID      0x1fc9   // NXP
#define PRODUCT_ID     0x0094   // NXP
#define ACM_CTRL_DTR   0x01
#define ACM_CTRL_RTS   0x02

/* We use a global variable to keep the device handle
 */
static struct libusb_device_handle *devh = NULL;

/* The Endpoint address are hard coded. You should use lsusb -v to find
 * the values corresponding to your device.
 */
static int ep_in_addr  = 0x82;
static int ep_out_addr = 0x03;


U16 smComVCom_Open(const char *connectString);
void write_char(unsigned char c)
{
    /* To send a char to the device simply initiate a bulk_transfer to the
     * Endpoint with address ep_out_addr.
     */
    int actual_length;
    if (libusb_bulk_transfer(devh, ep_out_addr, &c, 1,
                             &actual_length, 0) < 0) {
        fprintf(stderr, "Error while sending char\n");
    }
}

int read_chars(unsigned char * data, int size)
{
    /* To receive characters from the device initiate a bulk_transfer to the
     * Endpoint with address ep_in_addr.
     */
    int actual_length;
    int rc = libusb_bulk_transfer(devh, ep_in_addr, data, size, &actual_length,
                                  0);
    if (rc == LIBUSB_ERROR_TIMEOUT) {
        printf("timeout (%d)\n", actual_length);
        return -1;
    } else if (rc < 0) {
        fprintf(stderr, "Error while waiting for char\n");
        return -1;
    }

    return actual_length;
}

#define CHECK_ON_ATR

#define REMOTE_JC_SHELL_HEADER_LEN             (4)
#define REMOTE_JC_SHELL_MSG_TYPE_APDU_DATA  (0x01)

#include "sm_apdu.h"
#define MAX_BUF_SIZE                (MAX_APDU_BUF_LENGTH)

static U8 Header[2] = {0x01,0x00};
static U8 sockapdu[MAX_BUF_SIZE];
static U8 response[MAX_BUF_SIZE];
static U8 *pCmd = (U8*) &sockapdu;
static U8 *pRsp = (U8*) &response;

U32 smComSerial_Transceive(apdu_t * pApdu)
{
    int retval;
#if defined(LOG_SOCK)
    int i;
#endif
    U32 txLen = 0;
    U32 expectedLength = 0;
    U32 totalReceived = 0;
    U8 lengthReceived = 0;

    assert(pApdu != NULL);

    pApdu->rxlen = 0;
   // TODO (?): adjustments on Le and Lc for SCP still to be done
    memset(sockapdu, 0x00, MAX_BUF_SIZE);
    memset(response, 0x00, MAX_BUF_SIZE);

    // remote JC Terminal header construction
    txLen = pApdu->buflen;
    memcpy(pCmd, Header, sizeof(Header));
    pCmd[2] = (txLen& 0xFF00)>>8;
    pCmd[3] = txLen & 0xFF;
    memcpy(&pCmd[4], pApdu->pBuf, pApdu->buflen);
    pApdu->buflen += 4; /* header & length */

#ifdef LOG_SOCK
    sm_printf(CONSOLE, "   send: ");
    for (i=4; i < (txLen+4); i++)
    {
       sm_printf(CONSOLE, "%02X", pCmd[i]);
    }
    sm_printf(CONSOLE, "\r\n");
#endif
    int actual_length;
    // retval = send(pSockCtx->sockfd, (const char*) pCmd, pApdu->buflen, 0);
    retval =libusb_bulk_transfer(devh, ep_out_addr, (unsigned char*) pCmd, pApdu->buflen,
                             &actual_length, 0);
    if (retval < 0)
    {
        fprintf(stderr,"Client: send() failed: error %i.\r\n", retval);
        return SMCOM_SND_FAILED;
    }

    expectedLength = REMOTE_JC_SHELL_HEADER_LEN; // remote JC shell header length

    while (totalReceived < expectedLength)
    {
        // retval = recv(pSockCtx->sockfd, (char*) &pRsp[totalReceived], MAX_BUF_SIZE, 0);
        retval = read_chars(&pRsp[totalReceived], MAX_BUF_SIZE);
        if (retval < 0)
        {
           fprintf(stderr,"Client: recv() failed: error %i.\r\n", retval);
        //    close(pSockCtx->sockfd);
           assert(0);
           return SMCOM_RCV_FAILED;
        }
        else
        {
            totalReceived += retval;
        }
        if ((totalReceived >= REMOTE_JC_SHELL_HEADER_LEN) && (lengthReceived == 0))
        {
            expectedLength += ((pRsp[2]<<8) | (pRsp[3]));
            lengthReceived = 1;
        }
    }
    retval = totalReceived;

    retval -= 4; // Remove the 4 bytes of the Remote JC Terminal protocol
    memcpy(pApdu->pBuf, &pRsp[4], retval);

#ifdef LOG_SOCK
    sm_printf(CONSOLE, "   recv: ");
    for (i=0; i < retval; i++)
    {
       sm_printf(CONSOLE, "%02X", pApdu->pBuf[i]);
    }
    sm_printf(CONSOLE, "\r\n");
#endif

    pApdu->rxlen = (U16) retval;
    // reset offset for subsequent response parsing
    pApdu->offset = 0;
    return SMCOM_OK;
}

U32 smComSerial_TransceiveRaw(U8 * pTx, U16 txLen, U8 * pRx, U32 * pRxLen)
{
    S32 retval;
    U32 answerReceived = 0;
    U32 len = 0;
#if defined(LOG_SOCK) || defined(DBG_LOG_SOCK)
    int i;
#endif
    U32 readOffset = 0;
    U8 headerParsed = 0;
    U8 correctHeader = 0;
    memset(sockapdu, 0x00, MAX_BUF_SIZE);
    memset(response, 0x00, MAX_BUF_SIZE);

    memcpy(pCmd, Header, 2);
    pCmd[2] = (txLen & 0xFF00)>>8;
    pCmd[3] = (txLen & 0x00FF);
    memcpy(&pCmd[4], pTx, txLen);
    txLen += 4; /* header + len */

#ifdef DBG_LOG_SOCK
    sm_printf(CONSOLE, "   full send: ");
    for (i=0; i < txLen; i++)
    {
        sm_printf(CONSOLE, "%02X", pCmd[i]);
    }
    sm_printf(CONSOLE, "\r\n");
#endif

    // retval = send(pSockCtx->sockfd, (const char*) pCmd, txLen, 0);
    int actual_length;
    retval =libusb_bulk_transfer(devh, ep_out_addr, (unsigned char*) pCmd, txLen,
                             &actual_length, 0);
    if (retval < 0)
    {
        sm_printf(CONSOLE, "Client: send() failed: error %i.\r\n", retval);
        return SMCOM_SND_FAILED;
    }
    else
    {
#ifdef DBG_LOG_SOCK
        sm_printf(CONSOLE, "Client: send() is OK.\r\n");
#endif
    }

#ifdef LOG_SOCK
    sm_printf(CONSOLE, "   send: ");
    for (i=4; i < txLen; i++)
    {
        sm_printf(CONSOLE, "%02X", pCmd[i]);
    }
    sm_printf(CONSOLE, "\r\n");
#endif

    retval = REMOTE_JC_SHELL_HEADER_LEN; // receive at least the JCTerminal header

    while ((retval > 0) || (answerReceived == 0))
    {
        retval = read_chars((char*) pRsp, MAX_BUF_SIZE);
         //recv(pSockCtx->sockfd, (char*) pRsp, MAX_BUF_SIZE, 0);

        if (retval < 0)
        {
           fprintf(stderr,"Client: recv() failed: error %i\r\n", retval);

        //    close(pSockCtx->sockfd);
           return SMCOM_RCV_FAILED;
        }
        else // data received
        {
            while (retval > 0) // parse all bytes
            {
                if (headerParsed == 1) // header already parsed; get data
                {
                    if (retval >= (S32) len)
                    {
                        if (correctHeader == 1)
                        {
                            memcpy(&pRx[0], &pRsp[readOffset], len);
                            answerReceived = 1;
                        }
                        else
                        {
                            // reset header parsed
                            readOffset += len;
                            headerParsed = 0;
                        }
                        retval -= len;

                        if (retval == 0) // no data left, reset readOffset
                        {
                            readOffset = 0;
                        }
                    }
                    else
                    {
                        // data too small according header => Error
                        fprintf(stderr,"Failed reading data %x %x\r\n", retval, len);
                        return SMCOM_RCV_FAILED;
                    }
                }
                else // parse header
                {
                    len = ((pRsp[readOffset + 2]<<8) | (pRsp[readOffset + 3]));

                    if (pRsp[readOffset] == REMOTE_JC_SHELL_MSG_TYPE_APDU_DATA)
                    {
                        // type correct => copy the data
                        retval -= REMOTE_JC_SHELL_HEADER_LEN;
                        if (retval > 0) // data left to read
                        {
                            readOffset += REMOTE_JC_SHELL_HEADER_LEN;
                        }
                        correctHeader = 1;
                    }
                    else
                    {
                        // type incorrect => skip the data as well and try again if data are left
                        readOffset += REMOTE_JC_SHELL_HEADER_LEN;
                        retval -= REMOTE_JC_SHELL_HEADER_LEN;
                        correctHeader = 0;
                    }
                    headerParsed = 1;
                }
            }
        }
    }

#ifdef LOG_SOCK
    sm_printf(CONSOLE, "   recv: ");
    for (i=0; i < len; i++)
    {
       sm_printf(CONSOLE, "%02X", pRx[i]);
    }
    sm_printf(CONSOLE, "\r\n");
#endif

    *pRxLen = len;

    return SMCOM_OK;
}


void init_libusb()
{
 int rc = libusb_init(NULL);
    if (rc < 0) {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(rc));
        exit(1);
    }

    /* Set debugging output to max level.
     */
    libusb_set_debug(NULL, 3);

    /* Look for a specific device and open it.
     */
    devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    if (!devh) {
        fprintf(stderr, "Error finding USB device\n");
        exit(1);// goto out;
    }

    /* As we are dealing with a CDC-ACM device, it's highly probable that
     * Linux already attached the cdc-acm driver to this device.
     * We need to detach the drivers from all the USB interfaces. The CDC-ACM
     * Class defines two interfaces: the Control interface and the
     * Data interface.
     */
    for (int if_num = 0; if_num < 2; if_num++) {
        if (libusb_kernel_driver_active(devh, if_num)) {
            libusb_detach_kernel_driver(devh, if_num);
        }
        rc = libusb_claim_interface(devh, if_num);
        if (rc < 0) {
            fprintf(stderr, "Error claiming interface: %s\n",
                    libusb_error_name(rc));
                exit(1);
            // goto out;
        }
    }

    /* Start configuring the device:
     * - set line state
     */
    rc = libusb_control_transfer(devh, 0x21, 0x22, ACM_CTRL_DTR | ACM_CTRL_RTS,
                                0, NULL, 0, 0);
    if (rc < 0) {
        fprintf(stderr, "Error during control transfer: %s\n",
                libusb_error_name(rc));
    }

    /* - set line encoding: here 9600 8N1
     * 9600 = 0x2580 ~> 0x80, 0x25 in little endian
     */
    unsigned char encoding[] = { 0x80, 0x25, 0x00, 0x00, 0x00, 0x00, 0x08 };
    rc = libusb_control_transfer(devh, 0x21, 0x20, 0, 0, encoding,
                                sizeof(encoding), 0);
    if (rc < 0) {
        fprintf(stderr, "Error during control transfer: %s\n",
                libusb_error_name(rc));
    }

     /* We can now start sending or receiving data to the device
     */
    unsigned char buf[65];
    int len;

    smCom_Init(smComSerial_Transceive, smComSerial_TransceiveRaw);

    
    // while(1) {
    //     write_char('t');
    //     len = read_chars(buf, 64);
    //     buf[len] = 0;
    //     fprintf(stdout, "Received: \"%s\"\n", buf);
    //     sleep(1);
    // }

//     libusb_release_interface(devh, 0);

// out:
//     if (devh)
//             libusb_close(devh);
//     libusb_exit(NULL);
        // return rc;

}


static U32 smComVCom_GetATR(U8* pAtr, U16* atrLen)
{
#define MTY 0
// #define NAD 0x21
#define NAD 0x00

    int retval;
#if defined(LOG_SOCK) || defined(DBG_LOG_SOCK)
    int i;
#endif
    U32 expectedLength = 0;
    U32 totalReceived = 0;
    U8 lengthReceived = 0;

    // wait 256 ms
    U8 ATRCmd[8] = {MTY, NAD, 0, 4, 0, 0, 1, 0};

#ifdef LOG_SOCK
    sm_printf(CONSOLE, "   send: ATR\r\n");
    for (i=0; i < sizeof(ATRCmd); i++)
    {
       sm_printf(CONSOLE, "%02X", ATRCmd[i]);
    }
    sm_printf(CONSOLE, "\r\n");
#endif

int actual_length;
    // retval = send(pSockCtx->sockfd, (const char*) ATRCmd, sizeof(ATRCmd), 0);
    retval =libusb_bulk_transfer(devh, ep_out_addr, (unsigned char*) ATRCmd, sizeof(ATRCmd),
                             &actual_length, 0);
    if (retval < 0)
    {
       fprintf(stderr,"Client: send() failed: error %i.\r\n", retval);
       return 0;
    }

    expectedLength = REMOTE_JC_SHELL_HEADER_LEN; // remote JC shell header length

    while (totalReceived < expectedLength)
    {
        U32 maxCommLength;
        if (lengthReceived == 0)
        {
            maxCommLength = REMOTE_JC_SHELL_HEADER_LEN - totalReceived;
        }
        else
        {
            maxCommLength = expectedLength - totalReceived;
        }

        // retval = recv(pSockCtx->sockfd, (char*) &pAtr[totalReceived], maxCommLength, 0);
        retval = read_chars((char*) &pAtr[totalReceived], 33);
        if (retval < 0)
        {
           fprintf(stderr,"Client: recv() failed: error %i.\r\n", retval);
        //    close(pSockCtx->sockfd);
           assert(0);
           return 0;
        }
        else
        {
            totalReceived += retval;
        }
        if ((totalReceived >= REMOTE_JC_SHELL_HEADER_LEN) && (lengthReceived == 0))
        {
            expectedLength += ((pAtr[2]<<8) | (pAtr[3]));
            lengthReceived = 1;
        }
    }
    retval = totalReceived;

#ifdef LOG_SOCK
    sm_printf(CONSOLE, "   full recv: ");
    for (i=0; i < retval; i++)
    {
       sm_printf(CONSOLE, "%02X", pAtr[i]);
    }
    sm_printf(CONSOLE, "\r\n");
#endif

    retval -= 4; // Remove the 4 bytes of the Remote JC Terminal protocol
    memmove(pAtr, pAtr + 4, retval);

#ifdef LOG_SOCK
    sm_printf(CONSOLE, "   recv: ");
    for (i=0; i < retval; i++)
    {
       sm_printf(CONSOLE, "%02X", pAtr[i]);
    }
    sm_printf(CONSOLE, "\r\n");
#endif

    *atrLen = (U16) retval;
    return 0;//retval;
}

U16 smComVCom_Open(const char *connectString)
{
    printf("Connecting %s\n", connectString);
   init_libusb();
    int pAtr[MAX_APDU_BUF_LENGTH];
    U16 atrLen;
    int nAtr = smComVCom_GetATR(pAtr, &atrLen);
    return 0;
}


#ifdef __cplusplus
}
#endif
#endif //_SCCOMSOCKET_H_
