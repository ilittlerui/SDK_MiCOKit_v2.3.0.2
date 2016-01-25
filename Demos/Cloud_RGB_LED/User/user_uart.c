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
#include "ZigBeeNetwork.h"
#include "ZigBeeZLL.h"
#include "ZigBeePDM.h"


#define user_uart_log(M, ...) custom_log("USER_UART", M, ##__VA_ARGS__)
#define user_uart_log_trace() custom_log_trace("USER_UART")

volatile ring_buffer_t  rx_buffer;
volatile uint8_t        rx_data[USER_UART_BUFFER_LENGTH];


extern tsZCB_Network sZCB_Network;

uint8_t u8recoverNode = 0;


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
#define CMD_BUF_NUM 5
static char cmdbuff[CMD_BUF_NUM][128];
static int cmdnum=0;

tsSL_Message zcbReceivedMessageQueue[ZCB_MAX_MESSAGE_QUEUES];

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


    //ZCB Msg Handle Thread		启动 ZCB Msg 处理线程
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "ZCB Msg Handle",
                                  ZCB_MessageHandle_thread, STACK_SIZE_ZCBMSG_HANDLE_THREAD,
                                  NULL );
    require_noerr_action( err, exit, user_uart_log("ERROR: Unable to start the zcbMsg hdl thread.") );


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



/****************************************************************************
* Function	: recv_buff_parser
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
int recv_buff_parser(char*buff)
{
    char* sptr=NULL;
    char* eptr=NULL;
    static char startbuf[128];
    static int startbuflength=0;
    char* restBuff;
    //int i=0;
    //int ret=-1;
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
    static uint8_t cmdIndex=0,msgQueIndex=0;
    memset(zcbReceivedMessageQueue,0x00,ZCB_MAX_MESSAGE_QUEUES * sizeof(tsSL_Message));

    while(1)
    {
        mico_thread_msleep(10);	//延时10ms

        memset(inDataBuffer,0x0,USER_UART_ONE_PACKAGE_LENGTH);		//清空接收数据缓冲

        recvlen = user_uartRecv(inDataBuffer, USER_UART_ONE_PACKAGE_LENGTH);//user uart 接收数据
        if (recvlen <= 0)
            continue;

        recv_buff_parser((char*)inDataBuffer);	//解析接收到的数据，把完整的数据包存储在 cmd 数组中
        msgQueIndex = 0;
        for(cmdIndex = 0; cmdIndex < cmdnum; cmdIndex++)	//逐条处理 cmd
        {
            /* Initialise buffer */
            sMessage.u16Type = 0x00;
            /* Initialise length to large value so CRC is skipped if end received */
            sMessage.u16Length = 0xFFFF;
            //1.get the date from ControlBridge
            if (eSL_ReadMessage(&sMessage.u16Type, &sMessage.u16Length, SL_MAX_MESSAGE_LENGTH, sMessage.au8Message,(unsigned char*)cmdbuff[cmdIndex],strlen(cmdbuff[cmdIndex])) == E_SL_OK)
            {
                user_uart_log("msg get,type:%d,length:%d",sMessage.u16Type,sMessage.u16Length);	//消息类型

                //if(0x8046 == sMessage.u16Type)
                //{
                ////    user_uart_log("%d",msgQueIndex);
                //}
                //if(0x4D == sMessage.u16Type)
                //{
                //    user_uart_log("%d",msgQueIndex);
                //}
                for(; msgQueIndex< ZCB_MAX_MESSAGE_QUEUES ; msgQueIndex++)
                {
                    if(0 == zcbReceivedMessageQueue[msgQueIndex].u16Type)
                    {
                        //user_uart_log("%d saved at %d",sMessage.u16Type,msgQueIndex);
                        zcbReceivedMessageQueue[msgQueIndex].u16Type = sMessage.u16Type;
                        zcbReceivedMessageQueue[msgQueIndex].u16Length = sMessage.u16Length;
                        memcpy(zcbReceivedMessageQueue[msgQueIndex].au8Message,sMessage.au8Message,sMessage.u16Length);
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
                if(msgQueIndex == ZCB_MAX_MESSAGE_QUEUES)
                    user_uart_log("no space for Msg");
            }
            else
            {
                user_uart_log("msg not correct");
                for(cmdIndex=0; cmdbuff[cmdnum][cmdIndex]!=0x0; cmdIndex++)
                {
                    printf("%x ",cmdbuff[cmdnum][cmdIndex]);
                }
                printf("\r\n");

            }
        }
        memset(cmdbuff[0], 0x0, sizeof(cmdbuff[0])* cmdnum);
        cmdnum = 0;
    }

exit:
    if(inDataBuffer) free(inDataBuffer);
}


/****************************************************************************
* Function	: uartHandle_thread
* Description	: 处理 UART 消息
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void ZCB_MessageHandle_thread(void *inContext)
{
    uint8_t i;
    uint8_t zigbee_device_announce_flag = 255;
    //uint8_t zigbee_device_current_recover = ZIGBEE_NODE_MAX;
    uint8_t zigbee_device_recover_index = 0;
    //uint8_t zigbee_device_isRecoering = 0;
    uint8_t zigbee_device_hasStart = 0;
#pragma pack(1)
    tsZCB_NodePDM NodePdm_Temp;
#pragma pack()
    while(1)
    {
        //恢复节点信息
        //1.每隔 1s 执行一次设备announce	如果是第一次上电???
        if(zigbee_device_hasStart && ((--zigbee_device_announce_flag)==0))
        {
            //查看 node status      :查看 Flash 中的记录,恢复节点信息///////////////////////////
            uint16_t u16nodeStatus = 0;
            uint16_t u16nodeAliveStatus = 0;
            uint8_t *message = malloc(11);
            ePDM_GetNodeStatus(&u16nodeStatus);
            ePDM_GetNodeAliveStatus(&u16nodeAliveStatus);

            //查看节点alive情况，如果节点在网络中并且不在线,那么执行 device announce
            if(zigbee_device_recover_index >= ZIGBEE_NODE_MAX)
            {
                zigbee_device_recover_index = 0;
            }

            for(; zigbee_device_recover_index<ZIGBEE_NODE_MAX; zigbee_device_recover_index++)
            {
                //在网络中,但是未在线的设备		///不知道设备id?????????????
                if((((1<<zigbee_device_recover_index)& u16nodeStatus)==0) &&  (((1<<zigbee_device_recover_index)& u16nodeAliveStatus)!=0))
                {
                    memset((void*)(&NodePdm_Temp),0x00,ZIGBEE_NODE_LENGTH);
                    ePDM_ReadOneNode(&NodePdm_Temp,zigbee_device_recover_index);
                    if((NodePdm_Temp.u16DeviceID == 0x00)  || (NodePdm_Temp.u8MacCapability != 0x80))
                    {
                        user_uart_log("try to recover one node:%d",zigbee_device_recover_index);
                        user_uart_log("u16ShortAddress:%x",NodePdm_Temp.u16ShortAddress);
                        user_uart_log("u32IEEEAddressH:%x",NodePdm_Temp.u32IEEEAddressH);
                        user_uart_log("u32IEEEAddressL:%x",NodePdm_Temp.u32IEEEAddressL);
                        user_uart_log("device id:%d",NodePdm_Temp.u16DeviceID);
                        NodePdm_Temp.u16ShortAddress = ntohs(NodePdm_Temp.u16ShortAddress);
                        NodePdm_Temp.u32IEEEAddressH = ntohl(NodePdm_Temp.u32IEEEAddressH);
                        NodePdm_Temp.u32IEEEAddressL = ntohl(NodePdm_Temp.u32IEEEAddressL);
                        memcpy(message,&NodePdm_Temp.u16ShortAddress,2);
                        memcpy(message+2,&NodePdm_Temp.u32IEEEAddressH,4);
                        memcpy(message+6,&NodePdm_Temp.u32IEEEAddressL,4);
                        memcpy(message+10,&NodePdm_Temp.u8MacCapability,1);
                        //zigbee_device_current_recover = i;
                        ZCB_HandleDeviceAnnounce(NULL, 11, message,NodePdm_Temp.u16DeviceID);
                        zigbee_device_announce_flag = 255;
                        zigbee_device_recover_index++;
                        break;
                    }
                    else
                    {
                        user_uart_log("find enddevice,no recover");
                    }
                }
            }
            if(message)
                free(message);
        }

        mico_thread_msleep(10);	//延时20ms
        for(i = 0; i<ZCB_MAX_MESSAGE_QUEUES; i++)
        {
            switch(zcbReceivedMessageQueue[i].u16Type)
            {
                case 0:
                    //user_uart_log("msg empty");
                    break;
                case E_SL_MSG_LOG:						//0x8001
                    user_uart_log("log msg");
                    zcbReceivedMessageQueue[i].u16Type = 0;
                    break;
                case E_SL_MSG_NODE_CLUSTER_LIST:		//0x8003
                    //user_uart_log("nod cluster");
                    ZCB_HandleNodeClusterList(NULL,zcbReceivedMessageQueue[i].u16Length,zcbReceivedMessageQueue[i].au8Message);
                    zcbReceivedMessageQueue[i].u16Type = 0;
                    break;
                case E_SL_MSG_NODE_ATTRIBUTE_LIST:		//0x8004
                    //user_uart_log("nod atr");
                    ZCB_HandleNodeClusterAttributeList(NULL,zcbReceivedMessageQueue[i].u16Length,zcbReceivedMessageQueue[i].au8Message);
                    zcbReceivedMessageQueue[i].u16Type = 0;
                    break;
                case E_SL_MSG_NODE_COMMAND_ID_LIST:		//0x8005
                    //user_uart_log("nod cmd");
                    ZCB_HandleNodeCommandIDList(NULL,zcbReceivedMessageQueue[i].u16Length,zcbReceivedMessageQueue[i].au8Message);
                    zcbReceivedMessageQueue[i].u16Type = 0;
                    break;
                case E_SL_MSG_RESTART_PROVISIONED:		//0x8006
                    user_uart_log("NoN Factory new Restart");
                    //非初始化重启
                    //1.当前设备的状态:不在线  一个一个改??
                    ePDM_UpdateNodeAliveStatus(0xFFFF);
                    ZCB_HandleRestartProvisioned(NULL,zcbReceivedMessageQueue[i].u16Length,zcbReceivedMessageQueue[i].au8Message);
                    zigbee_device_hasStart = 1;
                    zcbReceivedMessageQueue[i].u16Type = 0;
                    break;
                case E_SL_MSG_RESTART_FACTORY_NEW:		//0x8007
                    user_uart_log("Factory New Restart");
                    ZCB_HandleRestartFactoryNew(NULL,zcbReceivedMessageQueue[i].u16Length,zcbReceivedMessageQueue[i].au8Message);
                    MicoFlashErase( MICO_PARTITION_ZIGBEEPDM_TEMP, 0x0, 256);
                    mico_thread_msleep(20);
                    eZCB_ConfigureControlBridge();
                    zigbee_device_hasStart = 1;
                    zcbReceivedMessageQueue[i].u16Type = 0;
                    break;
                case E_SL_MSG_LEAVE_INDICATION:			//0x8048
                    user_uart_log("device Leave");
                    uint64_t    u64IEEEAddress;
                    ZCB_HandleDeviceLeave(NULL,zcbReceivedMessageQueue[i].u16Length,zcbReceivedMessageQueue[i].au8Message);
                    zcbReceivedMessageQueue[i].u16Type = 0;

                    memcpy(&u64IEEEAddress,zcbReceivedMessageQueue[i].au8Message,8);
                    user_uart_log("ieee addr 0x%016llX",u64IEEEAddress);
                    //u64IEEEAddress = ntoh64(u64IEEEAddress);
                    ePDM_DelOneNode(&u64IEEEAddress);
                    break;
                case E_SL_MSG_NETWORK_JOINED_FORMED:	//0x8024
                    ZCB_HandleNetworkJoined(NULL,zcbReceivedMessageQueue[i].u16Length,zcbReceivedMessageQueue[i].au8Message);
                    zcbReceivedMessageQueue[i].u16Type = 0;
                    break;
                case E_SL_MSG_MATCH_DESCRIPTOR_RESPONSE://0x8046
                    user_uart_log("match resp");
                    ZCB_HandleMatchDescriptorResponse(NULL,zcbReceivedMessageQueue[i].u16Length,zcbReceivedMessageQueue[i].au8Message);
                    zcbReceivedMessageQueue[i].u16Type = 0;
                    user_uart_log("match resp over");
                    break;
                case E_SL_MSG_DEFAULT_RESPONSE:			//0x8101
                    user_uart_log("default response");
                    zcbReceivedMessageQueue[i].u16Type = 0;
                    break;
                case E_SL_MSG_ATTRIBUTE_REPORT:			//0x8102
                    user_uart_log("attr report");
                    ZCB_HandleAttributeReport(NULL,zcbReceivedMessageQueue[i].u16Length,zcbReceivedMessageQueue[i].au8Message);
                    zcbReceivedMessageQueue[i].u16Type = 0;
                    user_uart_log("attr report done");
                    break;
                case E_SL_MSG_DEVICE_ANNOUNCE:			//0x004D
                    user_uart_log("device anc");

                    //ZCB_HandleDeviceAnnounce(NULL,zcbReceivedMessageQueue[i].u16Length,zcbReceivedMessageQueue[i].au8Message,0x00);
                    //收到device announce
                    //1.保存设备信息到 flash 中
                    //2.如何处理 rejoin 时announce的设备信息???????????? 待处理=============================

                    ePDM_SaveDeviceAnnounce(zcbReceivedMessageQueue[i].au8Message);//保存一个节点:根据 ZCB_Node

                    zcbReceivedMessageQueue[i].u16Type = 0;
                    user_uart_log("device anc done");
                    break;
                case E_SL_MSG_IAS_ZONE_STATUS_CHANGE_NOTIFY:	//0x8401
                    user_uart_log("get ias change notify");
                    ZCB_HandleIASZoneStatusChangeNotify(NULL,zcbReceivedMessageQueue[i].u16Length,zcbReceivedMessageQueue[i].au8Message);

                    zcbReceivedMessageQueue[i].u16Type = 0;
                    break;
                //case E_SL_MSG_DATA_INDICATION:
                //    break;
                default:
                    //user_uart_log("not zcb initiative msg");
                    break;
            }
        }
        //user_uart_log("zcbMsg Hdl over");

    }

}

