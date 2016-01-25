/**
******************************************************************************
* @file    user_main.c
* @author  Eshen Wang
* @version V1.0.0
* @date    14-May-2015
* @brief   user main functons in user_main thread.
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

#include "mico.h"
#include "MicoFogCloud.h"
#include "Debug.h"
#include "json_c/json.h"
#include "user_uart.h"
#include "ZigbeeControlBridge.h"
#include "ZigBeeNetwork.h"
#include "ZigBeePDM.h"
#include "ZigBeeSerialLink.h"
#include "ZigBeeZLL.h"
#include "ZigBeeAttributeReport.h"

/* User defined debug log functions
 * Add your own tag like: 'USER', the tag will be added at the beginning of a log
 * in MICO debug uart, when you call this function.
 */
#define user_log(M, ...) custom_log("USER", M, ##__VA_ARGS__)
#define user_log_trace() custom_log_trace("USER")


//#pragma pack(show)

void vZCB_ReportNodes(app_context_t * const app_context );
void vZCB_ReportNodes1(app_context_t * const app_context );

extern tsSL_Message zcbReceivedMessageQueue[ZCB_MAX_MESSAGE_QUEUES];


/* user main function, called by AppFramework after system init done && wifi
 * station on in user_main thread.
 */
OSStatus user_main( app_context_t * const app_context )
{
    user_log_trace();
    OSStatus err = kUnknownErr;
    fogcloud_msg_t *recv_msg = NULL;
    json_object *recv_json_object = NULL;
    json_object *recv_json_object_temp= NULL;
    OSStatus eStatus = 0;

    require(app_context, exit);

    /*ZCB Init*/
    user_log("ZCB Init");
    eZCB_Init(app_context);

    /*User Uart Init*/
    user_log("User uart Init");
    err = user_uartInit();
    require_noerr_action( err, exit, user_log("ERROR: user_uartInit err = %d.", err));
    //testAttribute();
    //try to connect ZCB
    eZCB_EstablishComms();


    while(1)
    {
        mico_thread_msleep(200);	//延时200ms
        // check fogcloud connect status	检测 fogcloud 连接状态
        if(!app_context->appStatus.fogcloudStatus.isCloudConnected)
        {
            continue;
        }

        /* get a msg pointer, points to the memory of a msg:
         * msg data format: recv_msg->data = <topic><data>
         */
        err = MiCOFogCloudMsgRecv(app_context, &recv_msg, 100);	//接收 fogCloud 数据到recv_msg
        if(kNoErr == err)
        {
            // debug log in MICO dubug uart
            user_log("Cloud => Module: topic[%d]=[%.*s]\tdata[%d]=[%.*s]",
                     recv_msg->topic_len, recv_msg->topic_len, recv_msg->data,
                     recv_msg->data_len, recv_msg->data_len, recv_msg->data + recv_msg->topic_len);

            // parse json data from the msg, get led control value	解析到 json 数据
            recv_json_object = json_tokener_parse((const char*)(recv_msg->data + recv_msg->topic_len));
            if (NULL != recv_json_object)
            {
                json_object_object_foreach(recv_json_object, key, val)
                {
                    if(!strcmp(key, "11"))
                    {
                        user_log("Receive zigbee cmd");

                        if(strncmp("nodes",(char*)json_object_get_string(val),json_object_get_string_len(val)) == 0)
                        {
                            eStatus = E_ZCB_OK;
                            user_log("nodes");

                            //DBG_PrintDetailNode(&sZCB_Network.sNodes);
                            //get node's info
                            //send to app
                            vZCB_ReportNodes1(NULL);

                        }
                        else if(strncmp((char*)"init",(char*)json_object_get_string(val),json_object_get_string_len(val))==0)
                        {
                            user_log("init cmd");
                            eStatus = eZCB_FactoryNew();
                            if (eStatus != E_ZCB_OK)
                            {
                                user_log("zigbee com send err!");
                            }
                            else
                            {
                                user_log("com send ok!");
                            }
                        }
                        else if(strncmp((char*)"permit",(char*)json_object_get_string(val),json_object_get_string_len(val))==0)
                        {
                            eStatus = E_ZCB_OK;
                            user_log("permit device join");
                            eStatus = eZCB_SetPermitJoining(60);
                        }
                        else if(strncmp((char*)"flashnode",(char*)json_object_get_string(val),json_object_get_string_len(val))==0)
                        {
                            user_log("flash Node");
                            eStatus = E_ZCB_OK;
                            ePDM_DisplayAllNode();
                        }
                        else if(strncmp((char*)"msg",(char*)json_object_get_string(val),json_object_get_string_len(val))==0)
                        {
                            user_log("list msgQue");
                            eStatus = E_ZCB_OK;
                            uint8_t i=0;
                            while(i <ZCB_MAX_MESSAGE_QUEUES)
                            {
                                if(zcbReceivedMessageQueue[i].u16Type !=0)
                                {
                                    user_log("msg %d type %d",i,zcbReceivedMessageQueue[i].u16Type);
                                }
                                i++;
                            }
                        }
                        else if(strncmp((char*)"reportlist",(char*)json_object_get_string(val),json_object_get_string_len(val))==0)
                        {
                            user_log("list report");
                            eStatus = E_ZCB_OK;
                            eZCB_LogAllAttribute();
                        }
						else if(strncmp((char*)"factorynew",(char*)json_object_get_string(val),json_object_get_string_len(val))==0)
						{
							user_log("factory new");
							eZCB_Finish();
							eZCB_Init();
							eStatus = eZCB_FactoryNew();
						}
                        else
                        {
                            user_log("err cmd");
                        }

                    }
                    else if(!strcmp(key, "device_control"))
                    {
                        user_log("device_control");

                        recv_json_object_temp = val;

                        if (NULL != recv_json_object_temp)
                        {
                            uint32_t ieee_addr_h;
                            uint32_t ieee_addr_l;
                            uint16_t short_addr ;
                            uint16_t device_id ;
                            uint8_t endpoint_id = 0;
                            boolean onoff ;
                            json_object_object_foreach(recv_json_object_temp, key, val)
                            {
                                if(!strcmp(key, "Zigbee_longAddress_H"))
                                {
                                    ieee_addr_h = 0xFFFFFFFF & json_object_get_int64(val);
                                    //user_log("long addr h %d",ieee_addr_h);
                                }
                                else if(!strcmp(key, "Zigbee_longAddress_L"))
                                {
                                    ieee_addr_l = 0xFFFFFFFF & json_object_get_int64(val);
                                    //user_log("long addr l %d",ieee_addr_l);
                                }
                                else if(!strcmp(key, "Zigbee_shortAddress"))
                                {
                                    short_addr = 0xFFFF & json_object_get_int(val);
                                    //user_log("short_addr %d",short_addr);
                                }
                                else if(!strcmp(key, "Zigbee_deviceID"))
                                {
                                    device_id = 0xFFFF & json_object_get_int(val);
                                    //user_log("device_id %d",device_id);
                                }
                                else if(!strcmp(key, "Zigbee_OnOff"))
                                {
                                    onoff = json_object_get_boolean(val);
                                    //user_log("onoff cmd %d",onoff);
                                }
                                else if(!strcmp(key, "Zigbee_endpointID"))
                                {
                                    endpoint_id = json_object_get_int(val);
                                    //user_log("onoff cmd %d",onoff);
                                }
                                else
                                {
                                    user_log("err key");
                                }
                            }
                            // deal with cmd
                            tsZCB_Node * psZCBNode;
                            eStatus = E_ZCB_OK;
                            //找到要控制的节点
                            psZCBNode = psZCB_FindNodeShortAddress(short_addr);
                            if(psZCBNode == NULL)
                            {
                                user_log("not find node");
                            }
                            else
                            {
                                //发送控制命令
                                eStatus = eZBZLL_OnOff(psZCBNode,endpoint_id, 0, onoff);
                            }

                            recv_json_object_temp = NULL;
                        }
                        else
                        {

                        }
                    }
                    else if(!strcmp(key, "get_report"))
                    {
                        user_log("get_report");

                        recv_json_object_temp = val;

                        if (NULL != recv_json_object_temp)
                        {
                            uint16_t short_addr =0x00;
                            uint16_t device_id =0x00;
                            uint8_t endpoint_id = 0;
                            uint16_t cluster_id = 0;
                            uint16_t attribute_id =0;
                            json_object_object_foreach(recv_json_object_temp, key, val)
                            {
                                if(!strcmp(key, "Zigbee_shortAddress"))
                                {
                                    short_addr = 0xFFFF & json_object_get_int(val);
                                    user_log("short_addr %d",short_addr);
                                }
                                else if(!strcmp(key, "Zigbee_deviceID"))
                                {
                                    device_id = 0xFFFF & json_object_get_int(val);
                                    user_log("device_id %d",device_id);
                                }
                                else if(!strcmp(key, "Zigbee_endpointID"))
                                {
                                    endpoint_id = json_object_get_int(val);
                                    user_log("endpoint_id  %d",endpoint_id);

                                }
                                else if(!strcmp(key, "Zigbee_clusterID"))
                                {
                                    cluster_id = 0xFFFF & json_object_get_int(val);
                                    user_log("cluster_id  %d",cluster_id);

                                }
                                else if(!strcmp(key, "Zigbee_attributeID"))
                                {
                                    attribute_id= 0xFFFF & json_object_get_int(val);
                                    user_log("attribute_id  %d",attribute_id);

                                }

                                else
                                {
                                    user_log("err key");
                                }
                            }
                            //find the attribute and send the attribute to cloud
                            tsZCB_AttributeReport *psZCBAttribute = NULL;
                            psZCBAttribute = psZCB_FindAttribute(short_addr,endpoint_id,cluster_id,attribute_id);
                            if(psZCBAttribute)
                            {
                                eZCB_ReportAttributeToCloud(psZCBAttribute,NULL);
                            }
                            else
                            {
                                user_log("can't find attribute");
                            }



                            recv_json_object_temp = NULL;
                        }
                        else
                        {

                        }
                    }
                    else
                    {
                        user_log("other type Json");
                    }
                }


                // free memory of json object
                json_object_put(recv_json_object);
                recv_json_object = NULL;
            }

            // NOTE: must free msg memory after been used.
            if(NULL != recv_msg)
            {
                free(recv_msg);
                recv_msg = NULL;
            }
        }
        else
        {
        }

    }

exit:
    user_log("ERROR: user_main exit with err=%d", err);
    return err;
}

#if 0
/****************************************************************************
* Function	: vZCB_ReportNodes
* Description	: 上报 node
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void vZCB_ReportNodes(app_context_t * const app_context )
{
    struct json_object* json_object = NULL;
    struct json_object* send_json_objects = NULL;

    const char *upload_data = NULL;
    uint8_t key[5];
    tsZCB_Node *psNode = &sZCB_Network.sNodes;
    psNode  = psNode->psNext;


    while(psNode != NULL)
    {

        send_json_objects = json_object_new_object();
        if(NULL == send_json_objects)
        {
            user_log("create json object error!");
            //err = kNoMemoryErr;
        }
        else
        {
            // create json object to format upload data
            json_object = json_object_new_object();
            if(NULL == json_object)
            {
                user_log("create json object error!");
            }
            else
            {
                // add temperature/humidity data into a json oject  增加数据到 json 对象
                json_object_object_add(json_object, "Zigbee_longAddress_H", json_object_new_int(((psNode->u64IEEEAddress)&0xFFFFFFFF00000000)>>32));
                json_object_object_add(json_object, "Zigbee_longAddress_L", json_object_new_int((psNode->u64IEEEAddress)&0xFFFFFFFF));
                json_object_object_add(json_object, "Zigbee_shortAddress", json_object_new_int(psNode->u16ShortAddress));
                json_object_object_add(json_object, "Zigbee_deviceID", json_object_new_int(psNode->u16DeviceID));


                json_object_object_add(send_json_objects, "devicelist", json_object);


                // json object convert to json string
                upload_data = json_object_to_json_string(send_json_objects);

                if(NULL == upload_data)
                {
                    user_log("create upload data string error!");
                    //err = kNoMemoryErr;
                }
                else
                {
                    // check fogcloud connect status	检测 fogcloud 连接状态
                    if(app_context->appStatus.fogcloudStatus.isCloudConnected)
                    {
                        // upload data string to fogcloud, the seconde param(NULL) means send to defalut topic: '<device_id>/out'
                        // 上报数据到 fogcloud
                        MiCOFogCloudMsgSend(app_context, NULL, (unsigned char*)upload_data, strlen(upload_data));
                        user_log("upload data success! \t topic=%s/out",
                                 app_context->appConfig->fogcloudConfig.deviceId);
                    }


                }

                // free json object memory
                json_object_put(send_json_objects);
                send_json_objects = NULL;


                json_object = NULL;
            }
        }

        psNode = psNode->psNext;

    }
    return ;
}
#endif

void vZCB_ReportNodes1(app_context_t * const app_context )
{
    struct json_object* json_object = NULL;
    struct json_object* send_json_objects = NULL;
    struct json_object* actual_send_json_objects = NULL;
    const char *upload_data = NULL;
    uint8_t key[5]= {0};
    uint8_t deviceNum = 0;
#pragma pack(1)
    tsZCB_NodePDM NodePdm_Temp;
#pragma pack()

    actual_send_json_objects = json_object_new_object();
    if(NULL == actual_send_json_objects)
    {
        user_log("create json object error!");
        //err = kNoMemoryErr;
    }
    else
    {
        send_json_objects = json_object_new_object();
        if(NULL == send_json_objects)
        {
            user_log("create json object error!");
            //err = kNoMemoryErr;
        }
        else
        {
            int16_t i = 0;
            uint16_t NodeStatus = 0;
            uint16_t NodeAliveStatus = 0;
            ePDM_GetNodeStatus(&NodeStatus);
            ePDM_GetNodeAliveStatus(&NodeAliveStatus);
            user_log("Node status :%8x",NodeStatus);
            user_log("Node alive status :%8x",NodeAliveStatus);
            for(i=0; i<ZIGBEE_NODE_MAX; i++)
            {
                if(((1<<i)& NodeStatus)==0)
                {
                    json_object = json_object_new_object();
                    if(NULL == json_object)
                    {
                        user_log("create json object error!");
                        //err = kNoMemoryErr;
                    }
                    else
                    {
                        ePDM_ReadOneNode(&NodePdm_Temp,i);
                        // add temperature/humidity data into a json oject  增加数据到 json 对象
                        user_log("the i is:%d",i);
                        user_log("Node u8NodeStatus :%x",NodePdm_Temp.u8NodeStatus);
                        user_log("Node u16DeviceID :%d",NodePdm_Temp.u16DeviceID);
                        user_log("Node u16ShortAddress :%4x",NodePdm_Temp.u16ShortAddress);
                        user_log("Node u32IEEEAddressHigh :%8x",NodePdm_Temp.u32IEEEAddressH);
                        user_log("Node u32IEEEAddressLow :%8x",NodePdm_Temp.u32IEEEAddressL);
                        user_log("Node u8MacCapability :%x",NodePdm_Temp.u8MacCapability);
                        user_log("Node u16CustomData :%x",NodePdm_Temp.u16CustomData);
                        json_object_object_add(json_object, "Zigbee_longAddress_H", json_object_new_int(NodePdm_Temp.u32IEEEAddressH));
                        json_object_object_add(json_object, "Zigbee_longAddress_L", json_object_new_int(NodePdm_Temp.u32IEEEAddressL));
                        json_object_object_add(json_object, "Zigbee_shortAddress", json_object_new_int(NodePdm_Temp.u16ShortAddress));
                        json_object_object_add(json_object, "Zigbee_deviceID", json_object_new_int(NodePdm_Temp.u16DeviceID));
                        json_object_object_add(json_object, "Zigbee_deviceStatus", json_object_new_boolean((NodeAliveStatus&(1<<i))?0:1));
                        json_object_object_add(json_object, "Zigbee_customData", json_object_new_int(NodePdm_Temp.u16CustomData));

                        memset(key,0x0,5);
                        sprintf((char*)key,"d%d",deviceNum++);

                        json_object_object_add(send_json_objects, (char*)key, json_object);
                    }
                    json_object = NULL;

                }
            }

            json_object_object_add(actual_send_json_objects, "devicelist", send_json_objects);


            // json object convert to json string
            upload_data = json_object_to_json_string(actual_send_json_objects);

            if(NULL == upload_data)
            {
                user_log("create upload data string error!");
                //err = kNoMemoryErr;
            }
            else
            {
                // check fogcloud connect status	检测 fogcloud 连接状态
                if(app_context->appStatus.fogcloudStatus.isCloudConnected)
                {
                    // upload data string to fogcloud, the seconde param(NULL) means send to defalut topic: '<device_id>/out'
                    // 上报数据到 fogcloud
                    MiCOFogCloudMsgSend(app_context, NULL, (unsigned char*)upload_data, strlen(upload_data));
                    user_log("upload data success! \t topic=%s/out",
                             app_context->appConfig->fogcloudConfig.deviceId);
                }
            }

            // free json object memory
            json_object_put(actual_send_json_objects);
            send_json_objects = NULL;
        }

    }
    return ;
}


void vZCB_SendAlarm(app_context_t * const app_context )
{


}

void vZCB_SendZoneStatusChangeNotify(app_context_t * const app_context ,uint8_t u8Endpoint,uint16_t u16ClusterID,uint16_t	u16ZoneStatus,uint8_t u8ZoneId)
{
    struct json_object* send_json_objects = NULL;
    struct json_object* actual_send_json_objects = NULL;
    const char *upload_data = NULL;

    actual_send_json_objects = json_object_new_object();
    if(NULL == actual_send_json_objects)
    {
        user_log("create json object error!");
        //err = kNoMemoryErr;
    }
    else
    {
        send_json_objects = json_object_new_object();
        if(NULL == send_json_objects)
        {
            user_log("create json object error!");
            //err = kNoMemoryErr;
        }
        else
        {
            user_log("u8Endpoint :%d",u8Endpoint);
            user_log("u16ClusterID :%x",u16ClusterID);
            user_log("u16ZoneStatus :%x",u16ZoneStatus);
            user_log("u8ZoneId :%x",u8ZoneId);

            json_object_object_add(send_json_objects, "Zigbee_endpoint", json_object_new_int(u8Endpoint&0xFF));
            json_object_object_add(send_json_objects, "Zigbee_clusterID", json_object_new_int(u16ClusterID&0xFFFF));
            json_object_object_add(send_json_objects, "Zigbee_zoneStatus", json_object_new_int(u16ZoneStatus&0xFFFF));
            json_object_object_add(send_json_objects, "Zigbee_zoneId", json_object_new_int(u8ZoneId&0xFF));


            json_object_object_add(actual_send_json_objects, "zonestatus", send_json_objects);


            // json object convert to json string
            upload_data = json_object_to_json_string(actual_send_json_objects);

            if(NULL == upload_data)
            {
                user_log("create upload data string error!");
                //err = kNoMemoryErr;
            }
            else
            {
                // check fogcloud connect status	检测 fogcloud 连接状态
                if(app_context->appStatus.fogcloudStatus.isCloudConnected)
                {
                    // upload data string to fogcloud, the seconde param(NULL) means send to defalut topic: '<device_id>/out'
                    // 上报数据到 fogcloud
                    MiCOFogCloudMsgSend(app_context, NULL, (unsigned char*)upload_data, strlen(upload_data));
                    user_log("upload data success! \t topic=%s/out",
                             app_context->appConfig->fogcloudConfig.deviceId);
                }
            }

            // free json object memory
            json_object_put(actual_send_json_objects);
            send_json_objects = NULL;
        }

    }


}