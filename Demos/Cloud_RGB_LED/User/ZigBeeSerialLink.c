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

tsSL_Message  sMessage;
//static void *pvReaderThread(tsUtilsThread *psThreadInfo);

//static void *pvCallbackHandlerThread(tsUtilsThread *psThreadInfo);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern uint32_t u32ZCB_CurrentWaitSeq;
//extern int verbosity;


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

//static tsSerialLink sSerialLink;


typedef enum
{
    EMPTY,
    START_FLAG,
    UNKNOWN
} AckBufStat;

typedef enum
{
    INVALID_DATA=-1,
    NO_END_CODE = 0,
    NO_START_AND_END_CODE=1,
    NORMAL_CODE
} AckParserErrorCode;

#define CMD_ACK_BUF_NUM 3
static char cmdACKbuff[CMD_ACK_BUF_NUM][64];
static uint8_t cmdACKnum=0;


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
int recv_buff_parser_ACK(char*buff);


teSL_Status eSL_Init()
{
    //int i;

    //    if (eSerial_Init(cpSerialDevice, u32BaudRate, &sSerialLink.iSerialFd) != E_SERIAL_OK)
    //    {
    //        return E_SL_ERROR_SERIAL;
    //    }
    //
    /* Initialise serial link mutex */
    //mico_rtos_init_mutex(&sSerialLink.mutex);

    /* Initialise message callbacks */
    //    pthread_mutex_init(&sSerialLink.sCallbacks.mutex, NULL);
    //    sSerialLink.sCallbacks.psListHead = NULL;
    //
    /* Initialise message wait queue */
    //for (i = 0; i < SL_MAX_MESSAGE_QUEUES; i++)
    //{
    //    mico_rtos_init_mutex(&sSerialLink.asReaderMessageQueue[i].mutex);
    //    //pthread_cond_init(&sSerialLink.asReaderMessageQueue[i].cond_data_available, NULL);
    //    sSerialLink.asReaderMessageQueue[i].u16Type = 0;
    //}
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
        eStatus = eSL_MessageWait(E_SL_MSG_STATUS, 100, &u16Length, (void**)&psStatus);


        if (eStatus == E_SL_OK)
        {
            user_ZigbeeSerialLink_log("Status: %d, Sequence %d", psStatus->eStatus, psStatus->u8SequenceNo);
            eStatus = psStatus->eStatus;
            if (eStatus == E_SL_OK)
            {
                if (pu8SequenceNo)
                {
                    *pu8SequenceNo = psStatus->u8SequenceNo;
                }
            }
            if(psStatus != NULL)
                free(psStatus);
        }
    }
    else
    {
        user_ZigbeeSerialLink_log("Send Err");
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

    uint8_t u8DataToSend[128];

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
    uint32_t temptime=0;
    uint8_t recvlen = 0;
    //===================received ACK================================
    uint8_t *inDataBuffer = malloc(USER_UART_ONE_PACKAGE_LENGTH);	//接收数据缓冲
    require(inDataBuffer, exit);
    while(temptime < u32WaitTimeout)
    {
        temptime += 2;
        mico_thread_msleep(2);

        memset(inDataBuffer,0x0,USER_UART_ONE_PACKAGE_LENGTH);		//清空接收数据缓冲

        recvlen = user_uartRecv(inDataBuffer, USER_UART_ONE_PACKAGE_LENGTH);//user uart 接收数据
        if (recvlen <= 0)
            continue;

        recv_buff_parser_ACK((char*)inDataBuffer);	//解析接收到的数据，把完整的数据包存储在 ACKcmd 数组中

        //user_ZigbeeSerialLink_log("cmd num:%d",cmdACKnum);
        for(uint8_t i = 0; i<cmdACKnum; i++)	//逐条处理 ACKcmd
        {
            /* Initialise buffer */
            memset(&sMessage, 0, sizeof(tsSL_Message));	//初始化 message 缓冲
            /* Initialise length to large value so CRC is skipped if end received */
            sMessage.u16Length = 0xFFFF;

            if (eSL_ReadMessage(&sMessage.u16Type, &sMessage.u16Length, SL_MAX_MESSAGE_LENGTH, sMessage.au8Message,(unsigned char*)cmdACKbuff[i],strlen(cmdACKbuff[i])) == E_SL_OK)
            {

                user_ZigbeeSerialLink_log("Get Ack,temptime:%d, Type %d",temptime,u16Type);
                if(sMessage.u16Type == u16Type)
                {
                    if(u32ZCB_CurrentWaitSeq != 0)
                    {
                        uint32_t actualSeq = 0;
                        switch(u16Type)
                        {
                            case E_SL_MSG_DEFAULT_RESPONSE:
                            case E_SL_MSG_LEAVE_CONFIRMATION:
                            case E_SL_MSG_MANAGEMENT_LQI_RESPONSE:
                            case E_SL_MSG_IEEE_ADDRESS_RESPONSE:
                            case E_SL_MSG_NODE_DESCRIPTOR_RESPONSE:
                            case E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE:
                            case E_SL_MSG_READ_ATTRIBUTE_RESPONSE:
                            case E_SL_MSG_ADD_GROUP_RESPONSE:
                            case E_SL_MSG_REMOVE_GROUP_RESPONSE:
                            case E_SL_MSG_GET_GROUP_MEMBERSHIP_RESPONSE:
                            case E_SL_MSG_REMOVE_SCENE_RESPONSE:
                            case E_SL_MSG_STORE_SCENE_RESPONSE:
                            case E_SL_MSG_SCENE_MEMBERSHIP_RESPONSE:
                                actualSeq = sMessage.au8Message[0];
                                break;
                            case E_SL_MSG_DATA_INDICATION:
                                actualSeq = sMessage.au8Message[14];
                                break;
                            default :
                                user_ZigbeeSerialLink_log("other type");
                                break;
                        }
                        user_ZigbeeSerialLink_log("get seq :%d",actualSeq);
                        if(actualSeq==u32ZCB_CurrentWaitSeq)
                        {
                            user_ZigbeeSerialLink_log("certain Seq Ack get");
                            *pu16Length = sMessage.u16Length;
                            *ppvMessage = sMessage.au8Message;
                            if(inDataBuffer)
                                free(inDataBuffer);
                            u32ZCB_CurrentWaitSeq = 0;
                            return E_SL_OK;
                        }
                        else
                        {
                            continue;
                        }
                    }
                    else
                    {
                        *pu16Length = sMessage.u16Length;
                        *ppvMessage = sMessage.au8Message;
                        if(inDataBuffer)
                            free(inDataBuffer);
                        return E_SL_OK;
                    }
                }
            }
        }
        memset(cmdACKbuff[0], 0x0, sizeof(cmdACKbuff[0])* cmdACKnum);
        cmdACKnum = 0;
    }
    user_ZigbeeSerialLink_log("Error, no ack");
exit:
    if(inDataBuffer)
        free(inDataBuffer);
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


teSL_Status eSL_Destroy(void)
{
    //eUtils_ThreadStop(&sSerialLink.sSerialReader);

    //while (sSerialLink.sCallbacks.psListHead)
    //{
    //    eSL_RemoveListener(sSerialLink.sCallbacks.psListHead->u16Type, sSerialLink.sCallbacks.psListHead->prCallback);
    //}

    return E_SL_OK;
}


int recv_buff_parser_ACK(char*buff)
{
    char* sptr=NULL;
    char* eptr=NULL;
    static char startbuf[128];
    static int startbuflength=0;
    char* restBuff;
    //int i=0;
    //int ret=-1;
    static AckBufStat bufstat=EMPTY;   //0: no start&end flag 1: start flag 2: end flag

    if(buff == NULL)
        return INVALID_DATA;

    restBuff = buff;

    while(1)
    {
        //user_ZigbeeSerialLink_log("bufstat is :%d rest buff:",bufstat);
        //for(i=0; restBuff[i]!=0x0; i++)
        //{
        //    printf("%x ",restBuff[i]);
        //}
        //user_ZigbeeSerialLink_log("\r\n");

        sptr = strchr(restBuff,0x01);
        eptr = strchr(restBuff,0x03);

        if(bufstat == EMPTY)
        {
            if(sptr)//find first START_CODE
            {
                bufstat = START_FLAG;
                if(*(sptr+1))
                {
                    restBuff=sptr+1;//the START_CODE is not the end data
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else if(bufstat == START_FLAG)
        {
            if(eptr)
            {
                if((sptr==NULL)||((sptr - eptr)>0))//STARD_CODE not exist or START_CODE is after END_CODE, data is valid
                {
                    //printf("valid data between START_CODE and END_CODE!\r\n");
                    memcpy(startbuf+startbuflength,restBuff,eptr-restBuff+1);	//add end_code
                    memset(cmdACKbuff[cmdACKnum],0x0,sizeof(cmdACKbuff[cmdACKnum]));
                    cmdACKbuff[cmdACKnum][0] = 0x01;
                    memcpy(&(cmdACKbuff[cmdACKnum][1]),startbuf,startbuflength+eptr-restBuff+1);//copy START_CODE XXXXXX END_CODE TO recvbuff
                    //user_ZigbeeSerialLink_log("length:%d,cmdACKbuff[%d]=%s",startbuflength+eptr-restBuff+2,cmdACKnum,cmdACKbuff[cmdACKnum]);

                    //for(i=0; cmdACKbuff[cmdACKnum][i]!=0x0; i++)
                    //{
                    //    printf("%x ",cmdACKbuff[cmdACKnum][i]);
                    //}
                    //user_ZigbeeSerialLink_log("\r\n");

                    cmdACKnum++;
                    cmdACKnum %= CMD_ACK_BUF_NUM;

                    bufstat=EMPTY;
                    startbuflength = 0;
                    memset(startbuf,0x0,sizeof(startbuf));

                    if(*(eptr+1))
                    {
                        restBuff=eptr+1;//to find next START_CODE or END_CODE
                        continue;
                    }
                    else
                    {
                        break;//get the buff end,quit
                    }
                }
                else  //START_CODE XXX START_CODE XXXX END_CODE drop first START_CODE and data between the two START_CODE
                {
                    startbuflength = 0;
                    memset(startbuf,0x0,sizeof(startbuf));

                    if(*(sptr+1))
                    {
                        restBuff=sptr+1;//to find next START_CODE or END_CODE
                        continue;
                    }
                    else
                    {
                        break;//get the buff end,quit
                    }
                }
            }
            else
            {
                if(sptr)  // START_CODE XXXX START_CODE XXXX
                {
                    if(*(sptr+1))
                    {
                        restBuff=sptr+1;//drop first START_CODE
                        continue;
                    }
                    else
                    {
                        break;//get the buff end,quit
                    }
                }
                else  //START CODE XXXXXXXXXXXXXX   copy the data to buff
                {
                    memcpy(startbuf+startbuflength,restBuff,strlen(restBuff));//need to check strlen(sptr)
                    startbuflength+=strlen(restBuff);
                    break;//the buff end
                }
            }
        }
    }
    return NORMAL_CODE;
}


/*******************************************************************************
* INTERFACES
******************************************************************************/
