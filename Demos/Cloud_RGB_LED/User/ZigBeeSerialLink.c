/**
******************************************************************************
* @file    ZigBeeSerialLink.c
* @author  Sven Yang
* @version V0.2.0
* @date    26-August-2015
* @brief   This file contains the implementations of user uart And ZigBee SerialLink interfaces
******************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
******************************************************************************
*/

#include "MicoFogCloud.h"
#include "platform.h"
#include "platform_peripheral.h"
#include "mico_platform.h"
#include "platform_mcu_peripheral.h"
#include "ZigBeeSerialLink.h"
#include "user_uart.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/


#define user_ZigbeeSerialLink_log(M, ...) custom_log("ZigbeeSerialLink", M, ##__VA_ARGS__)
#define user_ZigbeeSerialLink_log_trace() custom_log_trace("ZigbeeSerialLink")

#define user_uart_log(M, ...) custom_log("USER_UART", M, ##__VA_ARGS__)
#define user_uart_log_trace() custom_log_trace("USER_UART")
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

uint8_t u8SL_CalculateCRC(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data);

//static void *pvReaderThread(tsUtilsThread *psThreadInfo);

//static void *pvCallbackHandlerThread(tsUtilsThread *psThreadInfo);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


//extern int verbosity;


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

static tsSerialLink sSerialLink;


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/



teSL_Status eSL_Init()
{
    int i;

    //    if (eSerial_Init(cpSerialDevice, u32BaudRate, &sSerialLink.iSerialFd) != E_SERIAL_OK)
    //    {
    //        return E_SL_ERROR_SERIAL;
    //    }
    //
    /* Initialise serial link mutex */
    mico_rtos_init_mutex(&sSerialLink.mutex);

    /* Initialise message callbacks */
    //    pthread_mutex_init(&sSerialLink.sCallbacks.mutex, NULL);
    //    sSerialLink.sCallbacks.psListHead = NULL;
    //
    /* Initialise message wait queue */
    for (i = 0; i < SL_MAX_MESSAGE_QUEUES; i++)
    {
        mico_rtos_init_mutex(&sSerialLink.asReaderMessageQueue[i].mutex);
        //pthread_cond_init(&sSerialLink.asReaderMessageQueue[i].cond_data_available, NULL);
        sSerialLink.asReaderMessageQueue[i].u16Type = 0;
    }
    //
    //    /* Initialise callback queue */
    //    if (eUtils_QueueCreate(&sSerialLink.sCallbackQueue, SL_MAX_CALLBACK_QUEUES, 0) != E_UTILS_OK)
    //    {
    //        daemon_log(LOG_ERR, "Error creating callabck queue\n");
    //        return E_SL_ERROR;
    //    }
    //
    //    /* Start the callback handler thread */
    //    sSerialLink.sCallbackThread.pvThreadData = &sSerialLink;
    //    if (eUtils_ThreadStart(pvCallbackHandlerThread, &sSerialLink.sCallbackThread, E_THREAD_JOINABLE) != E_UTILS_OK)
    //    {
    //        daemon_log(LOG_ERR, "Failed to start callback handler thread");
    //        return E_SL_ERROR;
    //    }
    //
    //    /* Start the serial reader thread */
    //    sSerialLink.sSerialReader.pvThreadData = &sSerialLink;
    //    if (eUtils_ThreadStart(pvReaderThread, &sSerialLink.sSerialReader, E_THREAD_JOINABLE) != E_UTILS_OK)
    //    {
    //        daemon_log(LOG_ERR, "Failed to start serial reader thread");
    //        return E_SL_ERROR;
    //    }
    //
    return E_SL_OK;
}


/****************************************************************************
* Function	: eSL_SendMessage
* Description	:	发送消息
* Input Para	: u16Type,  u16Length, *pvMessage,*pu8SequenceNo
* Output Para	:
* Return Value:
****************************************************************************/
teSL_Status eSL_SendMessage(uint16_t u16Type, uint16_t u16Length, void *pvMessage, uint8_t *pu8SequenceNo)
{
    teSL_Status eStatus;

    /* Make sure there is only one thread sending messages to the node at a time. */
    //互斥锁，uart metux??

    eStatus = eSL_WriteMessage(u16Type, u16Length, (uint8_t *)pvMessage);

    if (eStatus == E_SL_OK)	//如果消息发送成功
    {
        /* Command sent successfully */
        //user_ZigbeeSerialLink_log("");
        uint16_t    u16Length;
        tsSL_Msg_Status sStatus;
        tsSL_Msg_Status *psStatus = &sStatus;

        sStatus.u16MessageType = u16Type;

        /* Expect a status response within 100ms  在100ms内等待状态响应*/
        eStatus = eSL_MessageWait(E_SL_MSG_STATUS, 300, &u16Length, (void**)&psStatus);


        //if (eStatus == E_SL_OK)
        //{
        //   user_ZigbeeSerialLink_log("Status: %d, Sequence %d\n", psStatus->eStatus, psStatus->u8SequenceNo);
        //    eStatus = psStatus->eStatus;
        //    if (eStatus == E_SL_OK)
        //    {
        //       if (pu8SequenceNo)
        //        {
        //            *pu8SequenceNo = psStatus->u8SequenceNo;
        //        }
        //    }
        //    free(psStatus);
        //}
    }

    return eStatus;
}


/****************************************************************************
*
* NAME: vSL_WriteRawMessage
*
* DESCRIPTION:
*
* PARAMETERS: Name        RW  Usage
*
* RETURNS:
* void
****************************************************************************/
static teSL_Status eSL_WriteMessage(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data)
{
    int n;
    OSStatus err = kUnknownErr;
    uint8_t u8CRC;
    uint8_t u8EscChar = SL_ESC_CHAR;
    uint8_t datalen = 0;
    uint8_t *u8DataToSend = malloc(2*u16Length*sizeof(uint8_t));
    u8CRC = u8SL_CalculateCRC(u16Type, u16Length, pu8Data);

    //user_ZigbeeSerialLink_log("(%d, %d, %02x)", u16Type, u16Length, u8CRC);

    //    if (verbosity >= 10)
    //   {
    //        char acBuffer[4096];
    //        int iPosition = 0, i;
    //
    //        iPosition = sprintf(&acBuffer[iPosition], "Host->Node 0x%04X (Length % 4d)", u16Type, u16Length);
    //        for (i = 0; i < u16Length; i++)
    //        {
    //            iPosition += sprintf(&acBuffer[iPosition], " 0x%02X", pu8Data[i]);
    //       }
    //        //daemon_log(LOG_DEBUG, "%s", acBuffer);
    //    }

    /* Send start character */
    //if (iSL_TxByte(true, SL_START_CHAR) < 0) return E_SL_ERROR;

    u8DataToSend[datalen++] = SL_START_CHAR;

    /* Send message type */
    //if (iSL_TxByte(false, (u16Type >> 8) & 0xff) < 0) return E_SL_ERROR;
    //if (iSL_TxByte(false, (u16Type >> 0) & 0xff) < 0) return E_SL_ERROR;
    if (((u16Type >> 8) & 0xff) < 0x10)
    {
        u8DataToSend[datalen++] = u8EscChar;
        u8DataToSend[datalen++] = ((u16Type >> 8) & 0xff)^0x10;
    }
    else
    {
        u8DataToSend[datalen++] = ((u16Type >> 8) & 0xff);
    }

    if (((u16Type >> 0) & 0xff) < 0x10)
    {
        u8DataToSend[datalen++] = u8EscChar;
        u8DataToSend[datalen++] = ((u16Type >> 0) & 0xff)^0x10;
    }
    else
    {
        u8DataToSend[datalen++] = ((u16Type >> 0) & 0xff);
    }


    /* Send message length */
    //if (iSL_TxByte(false, (u16Length >> 8) & 0xff) < 0) return E_SL_ERROR;
    //if (iSL_TxByte(false, (u16Length >> 0) & 0xff) < 0) return E_SL_ERROR;

    if (((u16Length >> 8) & 0xff) < 0x10)
    {
        u8DataToSend[datalen++] = u8EscChar;
        u8DataToSend[datalen++] = ((u16Length >> 8) & 0xff)^0x10;
    }
    else
    {
        u8DataToSend[datalen++] = ((u16Length >> 8) & 0xff);
    }

    if (((u16Length >> 0) & 0xff) < 0x10)
    {
        u8DataToSend[datalen++] = u8EscChar;
        u8DataToSend[datalen++] = ((u16Length >> 0) & 0xff)^0x10;
    }
    else
    {
        u8DataToSend[datalen++] = ((u16Length >> 0) & 0xff);
    }



    /* Send message checksum */
    //if (iSL_TxByte(false, u8CRC) < 0) return E_SL_ERROR;
    if (u8CRC < 0x10)
    {
        u8DataToSend[datalen++] = u8EscChar;
        u8DataToSend[datalen++] = u8CRC^0x10;
    }
    else
    {
        u8DataToSend[datalen++] = u8CRC;
    }


    /* Send message payload */
    for(n = 0; n < u16Length; n++)
    {
        //if (iSL_TxByte(false, pu8Data[n]) < 0) return E_SL_ERROR;

        if (pu8Data[n] < 0x10)
        {
            u8DataToSend[datalen++] = u8EscChar;
            u8DataToSend[datalen++] = pu8Data[n]^0x10;
        }
        else
        {
            u8DataToSend[datalen++] = pu8Data[n];
        }
    }

    /* Send end character */
    //if (iSL_TxByte(true, SL_END_CHAR) < 0) return E_SL_ERROR;
    u8DataToSend[datalen++] = SL_END_CHAR;

    err = MicoUartSend(UART_FOR_APP, u8DataToSend, datalen);
    free(u8DataToSend);

    if(err != kNoErr)
        return E_SL_ERROR;
    return E_SL_OK;
}




/****************************************************************************
* Function	: u8SL_CalculateCRC
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
static uint8_t u8SL_CalculateCRC(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data)
{
    int n;
    uint8_t u8CRC = 0;

    u8CRC ^= (u16Type >> 8) & 0xff;
    u8CRC ^= (u16Type >> 0) & 0xff;

    u8CRC ^= (u16Length >> 8) & 0xff;
    u8CRC ^= (u16Length >> 0) & 0xff;

    for(n = 0; n < u16Length; n++)
    {
        u8CRC ^= pu8Data[n];
    }
    return(u8CRC);
}


/****************************************************************************
* Function	: eSL_MessageWait
* Description	: Expect a status response within u32WaitTimeout ms
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teSL_Status eSL_MessageWait(uint16_t u16Type, uint32_t u32WaitTimeout, uint16_t *pu16Length, void **ppvMessage)
{
    int i;
    tsSerialLink *psSerialLink = &sSerialLink;
    unsigned int temptime=0;
    while(temptime<u32WaitTimeout)
    {
        temptime += 10;
        mico_thread_msleep(10);
        for (i = 0; i < SL_MAX_MESSAGE_QUEUES; i++)
        {
            //ser_ZigbeeSerialLink_log("Locking queue %d mutex", i);
            mico_rtos_lock_mutex(&psSerialLink->asReaderMessageQueue[i].mutex);
            //user_ZigbeeSerialLink_log("Acquired queue %d mutex", i);

            if (psSerialLink->asReaderMessageQueue[i].u16Type == u16Type)
            {
                //struct timeval sNow;
                //struct timespec sTimeout;

                user_ZigbeeSerialLink_log("Found free slot %d to wait for message 0x%04X", i, u16Type);


                *pu16Length = psSerialLink->asReaderMessageQueue[i].u16Length;
                *ppvMessage = psSerialLink->asReaderMessageQueue[i].pu8Message;

                /* Reset queue for next user */
                psSerialLink->asReaderMessageQueue[i].u16Type = 0;
                mico_rtos_unlock_mutex(&psSerialLink->asReaderMessageQueue[i].mutex);
                user_ZigbeeSerialLink_log("temptime:%d",temptime);
                return E_SL_OK;
                //            memset(&sNow, 0, sizeof(struct timeval));
                //            gettimeofday(&sNow, NULL);
                //            sTimeout.tv_sec = sNow.tv_sec + (u32WaitTimeout/1000);
                //            sTimeout.tv_nsec = (sNow.tv_usec + ((u32WaitTimeout % 1000) * 1000)) * 1000;
                //            if (sTimeout.tv_nsec > 1000000000)
                //            {
                //                sTimeout.tv_sec++;
                //                sTimeout.tv_nsec -= 1000000000;
                //            }
                //            mico_log("Time now    %lu s, %lu ns\n", sNow.tv_sec, sNow.tv_usec * 1000);
                //            mico_log("Wait until  %lu s, %lu ns\n", sTimeout.tv_sec, sTimeout.tv_nsec);
                //
                //            switch (pthread_cond_timedwait(&psSerialLink->asReaderMessageQueue[i].cond_data_available, &psSerialLink->asReaderMessageQueue[i].mutex, &sTimeout))
                //            {
                //                case (0):
                //                    mico_log("Got message type 0x%04x, length %d\n",
                //                                psSerialLink->asReaderMessageQueue[i].u16Type,
                //                                psSerialLink->asReaderMessageQueue[i].u16Length);
                //                    *pu16Length = psSerialLink->asReaderMessageQueue[i].u16Length;
                //                    *ppvMessage = psSerialLink->asReaderMessageQueue[i].pu8Message;
                //
                //                    /* Reset queue for next user */
                //                    psSerialLink->asReaderMessageQueue[i].u16Type = 0;
                //                    pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
                //                    return E_SL_OK;
                //
                //                case (ETIMEDOUT):
                //                    mico_log("Timed out\n");
                //                    /* Reset queue for next user */
                //                    psSerialLink->asReaderMessageQueue[i].u16Type = 0;
                //                    pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
                //                    return E_SL_NOMESSAGE;
                //                    break;
                //
                //                default:
                //                    /* Reset queue for next user */
                //                    psSerialLink->asReaderMessageQueue[i].u16Type = 0;
                //                    pthread_mutex_unlock(&psSerialLink->asReaderMessageQueue[i].mutex);
                //                    return E_SL_ERROR;
                //            }
            }
            else
            {
                mico_rtos_unlock_mutex(&psSerialLink->asReaderMessageQueue[i].mutex);
            }
        }
    }



    user_ZigbeeSerialLink_log("Error, no ack");

    return E_SL_NOMESSAGE;
}



/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
teSL_Status eSL_ReadMessage(uint16_t *pu16Type, uint16_t *pu16Length, uint16_t u16MaxLength, uint8_t *pu8Message,uint8_t *inDataBuffer,int recvlen)
{

    static teSL_RxState eRxState = E_STATE_RX_WAIT_START;
    static uint8_t u8CRC;
    uint8_t u8Data;
    static uint16_t u16Bytes;
    static bool bInEsc = FALSE;
    int recvlenindex = 0;

    for(recvlenindex=0; recvlenindex<recvlen; recvlenindex++)
    {
        u8Data = inDataBuffer[recvlenindex];
        //user_ZigbeeSerialLink_log("0x%02x", u8Data);
        switch(u8Data)
        {

        case SL_START_CHAR:
            u16Bytes = 0;
            bInEsc = FALSE;
            //user_ZigbeeSerialLink_log("RX Start");
            eRxState = E_STATE_RX_WAIT_TYPEMSB;
            break;

        case SL_ESC_CHAR:
            //user_ZigbeeSerialLink_log("Got ESC");
            bInEsc = TRUE;
            break;

        case SL_END_CHAR:
            //user_ZigbeeSerialLink_log("Got END");

            if(*pu16Length > u16MaxLength)
            {
                /* Sanity check length before attempting to CRC the message */
                //user_ZigbeeSerialLink_log("Length > MaxLength");
                eRxState = E_STATE_RX_WAIT_START;
                break;
            }

            if(u8CRC == u8SL_CalculateCRC(*pu16Type, *pu16Length, pu8Message))
            {
#if DBG_SERIALLINK
                int i;
                user_ZigbeeSerialLink_log("RX Message type 0x%04x length %d: { ", *pu16Type, *pu16Length);
                for (i = 0; i < *pu16Length; i++)
                {
                    user_ZigbeeSerialLink_log("0x%02x ", pu8Message[i]);
                }
                user_ZigbeeSerialLink_log("}");
#endif /* DBG_SERIALLINK */

                eRxState = E_STATE_RX_WAIT_START;
                return E_SL_OK;
            }
            //user_ZigbeeSerialLink_log("CRC BAD");
            break;

        default:
            if(bInEsc)
            {
                u8Data ^= 0x10;
                bInEsc = FALSE;
            }

            switch(eRxState)
            {

            case E_STATE_RX_WAIT_START:
                break;


            case E_STATE_RX_WAIT_TYPEMSB:
                *pu16Type = (uint16_t)u8Data << 8;
                eRxState++;
                break;

            case E_STATE_RX_WAIT_TYPELSB:
                *pu16Type += (uint16_t)u8Data;
                eRxState++;
                break;

            case E_STATE_RX_WAIT_LENMSB:
                *pu16Length = (uint16_t)u8Data << 8;
                eRxState++;
                break;

            case E_STATE_RX_WAIT_LENLSB:
                *pu16Length += (uint16_t)u8Data;
                //user_ZigbeeSerialLink_log("Length %d\n", *pu16Length);
                if(*pu16Length > u16MaxLength)
                {
                    //user_ZigbeeSerialLink_log("Length > MaxLength\n");
                    eRxState = E_STATE_RX_WAIT_START;
                }
                else
                {
                    eRxState++;
                }
                break;

            case E_STATE_RX_WAIT_CRC:
                //user_ZigbeeSerialLink_log("CRC %02x\n", u8Data);
                u8CRC = u8Data;
                eRxState++;
                break;

            case E_STATE_RX_WAIT_DATA:
                if(u16Bytes < *pu16Length)
                {
                    //user_ZigbeeSerialLink_log("Data\n");
                    pu8Message[u16Bytes++] = u8Data;
                }
                break;

            default:
                //user_ZigbeeSerialLink_log("Unknown state\n");
                eRxState = E_STATE_RX_WAIT_START;
            }
            break;
        }
    }

    return E_SL_NOMESSAGE;
}


/****************************************************************************
* Function	: eSL_MessageQueue
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
static teSL_Status eSL_MessageQueue(tsSerialLink *psSerialLink, uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Message)
{
    //    int i;
    //    for (i = 0; i < SL_MAX_MESSAGE_QUEUES; i++)
    //    {
    //        mico_rtos_lock_mutex(&psSerialLink->asReaderMessageQueue[i].mutex);	//lock
    //
    //        if (psSerialLink->asReaderMessageQueue[i].u16Type == u16Type)	//如果与 u16Type 类型相同
    //        {
    //            user_ZigbeeSerialLink_log("Found listener for message type 0x%04x in slot %d\n", u16Type, i);
    //
    //            if (u16Type == E_SL_MSG_STATUS)
    //            {
    //                tsSL_Msg_Status *psRxStatus = (tsSL_Msg_Status*)pu8Message;
    //                tsSL_Msg_Status *psWaitStatus = (tsSL_Msg_Status*)psSerialLink->asReaderMessageQueue[i].pu8Message;
    //
    //                /* Also check the type of the message that this is status to. */
    //                if (psWaitStatus)
    //                {
    //                    user_ZigbeeSerialLink_log("Status listener for message type 0x%04X, rx 0x%04X\n", psWaitStatus->u16MessageType, ntohs(psRxStatus->u16MessageType));
    //
    //                    if (psWaitStatus->u16MessageType != ntohs(psRxStatus->u16MessageType))
    //                    {
    //                        user_ZigbeeSerialLink_log("Not the status listener for this message\n");
    //                        mico_rtos_unlock_mutex(&psSerialLink->asReaderMessageQueue[i].mutex);
    //                        continue;
    //                    }
    //                }
    //            }
    //
    //            uint8_t  *pu8MessageCopy = malloc(u16Length);
    //            if (!pu8MessageCopy)
    //            {
    //                user_ZigbeeSerialLink_log("Memory allocation failure");
    //                return E_SL_ERROR_NOMEM;
    //            }
    //            memcpy(pu8MessageCopy, pu8Message, u16Length);
    //
    //
    //            //psSerialLink->asReaderMessageQueue[i].u16Length = u16Length;
    //            //psSerialLink->asReaderMessageQueue[i].pu8Message = pu8MessageCopy;
    //
    //            /* Signal data available 信号数据可用*/
    //            user_ZigbeeSerialLink_log("Unlocking queue %d mutex\n", i);
    //            mico_rtos_unlock_mutex(&psSerialLink->asReaderMessageQueue[i].mutex);
    //            //pthread_cond_broadcast(&psSerialLink->asReaderMessageQueue[i].cond_data_available);
    //            return E_SL_OK;
    //        }
    //        else
    //        {
    //            mico_rtos_unlock_mutex(&psSerialLink->asReaderMessageQueue[i].mutex);
    //        }
    //    }
    //    user_ZigbeeSerialLink_log("No listeners for message type 0x%04X\n", u16Type);
    return E_SL_NOMESSAGE;
}
/*******************************************************************************
* INTERFACES
******************************************************************************/
