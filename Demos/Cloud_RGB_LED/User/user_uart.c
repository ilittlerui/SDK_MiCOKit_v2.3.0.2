/**
******************************************************************************
* @file    uart.h
* @author  Eshen Wang
* @version V1.0.0
* @date    17-Mar-2015
* @brief   This file contains the implementations of uart interfaces for user.
  operation
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

#include <stdio.h>

#include "mico.h"
#include "user_uart.h"
#include "ZigBeeSerialLink.h"
#include "ZigbeeControlBridge.h"

#define user_uart_log(M, ...) custom_log("USER_UART", M, ##__VA_ARGS__)
#define user_uart_log_trace() custom_log_trace("USER_UART")

volatile ring_buffer_t  rx_buffer;
volatile uint8_t        rx_data[USER_UART_BUFFER_LENGTH];


typedef enum
{
    EMPTY,
    START_FLAG,
    UNKNOWN
} BufStat;

typedef enum
{
    INVALID_DATA=-1,
    NO_END_CODE = 0,
    NO_START_AND_END_CODE=1,
    NORMAL_CODE
} ParserErrorCode;
#define START_CODE 0x01
#define END_CODE 0x03
#define CMD_BUF_NUM 10
static char cmdbuff[CMD_BUF_NUM][128];
static int cmdnum=0;


/*******************************************************************************
* INTERFACES
******************************************************************************/

OSStatus user_uartInit(void)
{
    OSStatus err = kUnknownErr;
    mico_uart_config_t uart_config;

    //USART init
    uart_config.baud_rate    = 115200;
    uart_config.data_width   = DATA_WIDTH_8BIT;
    uart_config.parity       = NO_PARITY;
    uart_config.stop_bits    = STOP_BITS_1;
    uart_config.flow_control = FLOW_CONTROL_DISABLED;
    uart_config.flags = UART_WAKEUP_DISABLE;
    ring_buffer_init  ( (ring_buffer_t *)&rx_buffer, (uint8_t *)rx_data, USER_UART_BUFFER_LENGTH );

    MicoUartInitialize( USER_UART, &uart_config, (ring_buffer_t *)&rx_buffer );

    //USART receive thread		启动uart接收线程
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART Recv",
                                  uartRecv_thread, STACK_SIZE_USART_RECV_THREAD,
                                  NULL );
    require_noerr_action( err, exit, user_uart_log("ERROR: Unable to start the USART recv thread.") );
    return kNoErr;

exit:
    return err;
}

OSStatus user_uartSend(unsigned char *inBuf, unsigned int inBufLen)
{
    OSStatus err = kUnknownErr;

    if( (NULL == inBuf) || ( 0 == inBufLen) )
    {
        err = kParamErr;
        user_uart_log("ERROR: user_uartSend input params error!");
        goto exit;
    }

    user_uart_log("KIT => MCU:[%d]=%.*s", inBufLen, inBufLen, inBuf);

    err = MicoUartSend(USER_UART, inBuf, inBufLen);
    require_noerr_action( err, exit, user_uart_log("ERROR: send to USART error! err=%d", err) );
    return kNoErr;

exit:
    return err;
}

uint32_t user_uartRecv(unsigned char *outBuf, unsigned int getLen)
{
    unsigned int data_len = 0;

    if( (NULL == outBuf) || (0 == getLen) )
    {
        user_uart_log("ERROR: user_uartRecv input params error!");
        return 0;
    }

    if( MicoUartRecv( USER_UART, outBuf, getLen, USER_UART_RECV_TIMEOUT) == kNoErr)
    {
        data_len = getLen;
    }
    else
    {
        data_len = MicoUartGetLengthInBuffer( USER_UART );
        if(data_len)
        {
            MicoUartRecv(USER_UART, outBuf, data_len, USER_UART_RECV_TIMEOUT);
        }
        else
        {
            data_len = 0;
        }
    }

    return data_len;
}


int recv_buff_parser(char*buff)
{
    char* sptr=NULL;
    char* eptr=NULL;
    static char startbuf[128];
    static int startbuflength=0;
    char* restBuff;
    int i=0;
//	int ret=-1;
    static BufStat bufstat=EMPTY;   //0: no start&end flag 1: start flag 2: end flag

    if(buff == NULL)
        return INVALID_DATA;

    restBuff = buff;

    while(1)
    {
        //user_uart_log("bufstat is :%d rest buff:",bufstat);
        //for(i=0; restBuff[i]!=0x0; i++)
        //{
        //    printf("%x ",restBuff[i]);
        //}
        //user_uart_log("\r\n");

        sptr = strchr(restBuff,START_CODE);
        eptr = strchr(restBuff,END_CODE);

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
                if((sptr==NULL)||((sptr -eptr)>0))//STARD_CODE not exist or START_CODE is after END_CODE, data is valid
                {
                    //printf("valid data between START_CODE and END_CODE!\r\n");
                    memcpy(startbuf+startbuflength,restBuff,eptr-restBuff+1);	//add end_code
                    memset(cmdbuff[cmdnum],0x0,sizeof(cmdbuff[cmdnum]));
                    cmdbuff[cmdnum][0] = 0x01;
                    memcpy(&(cmdbuff[cmdnum][1]),startbuf,startbuflength+eptr-restBuff+1);//copy START_CODE XXXXXX END_CODE TO recvbuff
                    //user_uart_log("length:%d,cmdbuff[%d]=%s",startbuflength+eptr-restBuff+2,cmdnum,cmdbuff[cmdnum]);

                    //for(i=0; cmdbuff[cmdnum][i]!=0x0; i++)
                    //{
                    //    printf("%x ",cmdbuff[cmdnum][i]);
                    //}
                    //user_uart_log("\r\n");

                    cmdnum++;
                    cmdnum %= CMD_BUF_NUM;

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


/****************************************************************************
* Function	: uartRecv_thread
* Description	: uart Recv Thread	uart 接收线程
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void uartRecv_thread(void *inContext)
{
    user_uart_log_trace();
    //mico_Context_t *Context = inContext;
    int recvlen;
    tsSL_Message  sMessage;
    //OSStatus err = kUnknownErr;

    uint8_t *inDataBuffer = malloc(USER_UART_ONE_PACKAGE_LENGTH);	//接收数据缓冲
    require(inDataBuffer, exit);

    //uint8_t i=0;
    //int iHandled;
    while(1)
    {
        mico_thread_msleep(10);	//延时50ms
        memset(inDataBuffer,0x0,USER_UART_ONE_PACKAGE_LENGTH);		//清空接收数据缓冲
        // ======================get msg from uart=======================
        recvlen = user_uartRecv(inDataBuffer, USER_UART_ONE_PACKAGE_LENGTH);//user uart 接收数据
        if (recvlen <= 0)
            continue;

        //user_uart_log("UART => Module: [%d]=%.*s", recvlen, recvlen, inDataBuffer);
        //user_uart_log("UART => Module len: [%d]", recvlen);
        recv_buff_parser((char*)inDataBuffer);	//解析接收到的数据，把完整的数据包存储在 cmd 数组中

        for(uint8_t i = 0; i<cmdnum; i++)	//逐条处理 cmd
        {
            //user_uart_log("cmdbuff[%d],len :%d",i,strlen(cmdbuff[i]));
            /* Initialise buffer */
            memset(&sMessage, 0, sizeof(tsSL_Message));	//初始化 message 缓冲
            /* Initialise length to large value so CRC is skipped if end received */
            sMessage.u16Length = 0xFFFF;
            //1.get the date from ControlBridge
            if (eSL_ReadMessage(&sMessage.u16Type, &sMessage.u16Length, SL_MAX_MESSAGE_LENGTH, sMessage.au8Message,(unsigned char*)cmdbuff[i],strlen(cmdbuff[i])) == E_SL_OK)
            {
                user_uart_log("message get,type:%d",sMessage.u16Type);	//消息类型

                switch(sMessage.u16Type)
                {
                    case E_SL_MSG_NODE_CLUSTER_LIST:
                        ZCB_HandleNodeClusterList(NULL,sMessage.u16Length,sMessage.au8Message);
                        break;
                    case E_SL_MSG_NODE_ATTRIBUTE_LIST:
                        ZCB_HandleNodeClusterAttributeList(NULL,sMessage.u16Length,sMessage.au8Message);
                        break;
                    case E_SL_MSG_NODE_COMMAND_ID_LIST:
                        ZCB_HandleNodeCommandIDList(NULL,sMessage.u16Length,sMessage.au8Message);
                        break;
                    case E_SL_MSG_NETWORK_JOINED_FORMED:
                        ZCB_HandleNetworkJoined(NULL,sMessage.u16Length,sMessage.au8Message);
                        break;
                    case E_SL_MSG_DEVICE_ANNOUNCE:
                        ZCB_HandleDeviceAnnounce(NULL,sMessage.u16Length,sMessage.au8Message);
                        break;
                    case E_SL_MSG_LEAVE_INDICATION:
                        ZCB_HandleDeviceLeave(NULL,sMessage.u16Length,sMessage.au8Message);
                        break;
                    case E_SL_MSG_MATCH_DESCRIPTOR_RESPONSE:
                        ZCB_HandleMatchDescriptorResponse(NULL,sMessage.u16Length,sMessage.au8Message);
                        break;
                    case E_SL_MSG_RESTART_PROVISIONED:
                        ZCB_HandleRestartProvisioned(NULL,sMessage.u16Length,sMessage.au8Message);
                        break;
                    case E_SL_MSG_RESTART_FACTORY_NEW:
                        ZCB_HandleRestartFactoryNew(NULL,sMessage.u16Length,sMessage.au8Message);
                        break;
                    case E_SL_MSG_ATTRIBUTE_REPORT:
                        ZCB_HandleAttributeReport(NULL,sMessage.u16Length,sMessage.au8Message);
                        break;
                    default:
                        user_uart_log("default case");
                        break;
                }

//            //see if any threads are waiting for this message??????
//            for(i =0; i<SL_MAX_MESSAGE_QUEUES; i++)
//            {
//                if(sSerialLink.asReaderMessageQueue[i].u16Type == 0)
//                {
//                    mico_rtos_lock_mutex(&sSerialLink.asReaderMessageQueue[i].mutex);
//                    sSerialLink.asReaderMessageQueue[i].u16Type = sMessage.u16Type;
//                    sSerialLink.asReaderMessageQueue[i].u16Length = sMessage.u16Length;
//                    memcpy(sSerialLink.asReaderMessageQueue[i].pu8Message, sMessage.au8Message, sMessage.u16Length);
//                    mico_rtos_unlock_mutex(&sSerialLink.asReaderMessageQueue[i].mutex);
//                    break;
//                }
//            }

            }
            else
            {
                user_uart_log("aaa");
            }
//        //2.deal the date according the massage type
//
//        //3.send the result to cloud:set format

        }
        memset(cmdbuff[0], 0x0, sizeof(cmdbuff[0])* cmdnum);
        cmdnum = 0;

        // transfer ACK msg to cloud
        //err = MicoFogCloudMsgSend(Context, NULL, 0, inDataBuffer, recvlen);
        //if(kNoErr == err){
        // user_uart_log("Msg send to cloud success!");
        //}
        //else{
        //  user_uart_log("Msgsend to cloud failed! err=%d.", err);
        //}

    }

exit:
    if(inDataBuffer) free(inDataBuffer);
}

