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

#include "json_c/json.h"
#include "rgb_led/hsb2rgb_led.h"
#include "lcd/oled.h"
#include "user_uart.h"
#include "ZigbeeControlBridge.h"
#include "ZigBeeNetwork.h"

/* User defined debug log functions
 * Add your own tag like: 'USER', the tag will be added at the beginning of a log
 * in MICO debug uart, when you call this function.
 */
#define user_log(M, ...) custom_log("USER", M, ##__VA_ARGS__)
#define user_log_trace() custom_log_trace("USER")

//#pragma pack(show)

char cloudMsg[128]= {0};
uint8_t cloudMsgLen = 0;
uint8_t cloudMsgIncome = 0;

char uartMsg[128]= {0};
uint8_t uartMsgLen = 0;

uint8_t cmdHaveDone = 0;
uint8_t currentCmd = 0;



/* user main function, called by AppFramework after system init done && wifi
 * station on in user_main thread.
 */
OSStatus user_main( app_context_t * const app_context )
{
    user_log_trace();
    OSStatus err = kUnknownErr;
    fogcloud_msg_t *recv_msg = NULL;
    json_object *recv_json_object = NULL;

    /* rgb led control variants, use hsb color.
     * h -- hues
     * s -- saturation
     * b -- brightness
     */
    bool led_switch = false;
    int led_hues = 0;
    int led_saturation = 0;
    int led_brightness = 0;

    require(app_context, exit);

    hsb2rgb_led_init(); 	// rgb led init		初始化 RGB
    err = user_uartInit();		// uart init		初始化 user_UART
    require_noerr_action( err, exit, user_log("ERROR: user_uartInit err = %d.", err) );
    user_log("User uart Init");
    
    eZCB_Init(app_context);			//ZigBee ControlBridge Init		初始化zigbee协调器
    user_log("ZCB Init");
    struct json_object* send_json_object = NULL;
    const char *upload_data = NULL;

    //teZcbStatus eStatus;
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
                    if(!strcmp(key, "rgbled_switch"))
                    {
                        led_switch = json_object_get_boolean(val);
                    }
                    else if(!strcmp(key, "rgbled_hues"))
                    {
                        led_hues = json_object_get_int(val);
                    }
                    else if(!strcmp(key, "rgbled_saturation"))
                    {
                        led_saturation = json_object_get_int(val);
                    }
                    else if(!strcmp(key, "rgbled_brightness"))
                    {
                        led_brightness = json_object_get_int(val);
                    }
                    else if(!strcmp(key, "11"))
                    {
                        user_log("Receive zigbee cmd");

                        if(strlen(cloudMsg)==0)
                        {
                            cmdHaveDone = 0;
                            memset(cloudMsg,0x0,sizeof(cloudMsg));
                            cloudMsgLen=json_object_get_string_len(val);
                            cloudMsgIncome = 1;
                            strncpy((char*)cloudMsg, (char*)json_object_get_string(val), cloudMsgLen);
                        }
                        else
                        {
                            user_log("pre cmd is handling");
                        }

#if 0
                        if(strncmp((char*)"init",(char*)cloudMsg,json_object_get_string_len(val))==0)
                        {
                            user_log("init cmd");

                            eStatus = eZCB_FactoryNew();
                            if (eStatus != E_ZCB_OK)
                            {
                                user_log("zigbee com send err!");
                            }
                            //else if()
                            //{
                            //	user_log("pre com send ok!");
                            //}
                            else
                            {
                                user_log("pre com send ok!");
                                //send next cmd
                            }
                        }
                        else if(strncmp((char*)"device",(char*)cloudMsg,json_object_get_string_len(val))==0)
                        {
                            user_log("get device");
                        }
                        else if(strncmp((char*)"permit",(char*)cloudMsg,json_object_get_string_len(val))==0)
                        {
                            user_log("permit device join");
                            eZCB_SetPermitJoining(60);
                        }
                        else if(strncmp((char*)"nodes",(char*)cloudMsg,json_object_get_string_len(val))==0)
                        {
                            user_log("nodes");
                            DBG_PrintNode(&sZCB_Network.sNodes);
                        }
                        else if(strncmp((char*)"network",(char*)cloudMsg,json_object_get_string_len(val))==0)
                        {
                            user_log("network");
                            //DisplayZCBNetwork();
                        }
                        else
                        {
                            user_log("err cmd");
                        }

#endif


                    }
                }

                // control led
                if(led_switch)
                {
                    hsb2rgb_led_open(led_hues, led_saturation, led_brightness);  // open rgb led
                    OLED_ShowString(OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_3, "LED on          ");  // show cmd on LCD
                }
                else
                {
                    hsb2rgb_led_close();  // close led
                    OLED_ShowString(OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_3, "LED off         ");  // show cmd on LCD
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
            // update info on LCD
            OLED_ShowString(OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_2, "Demo RGB LED    ");  // clean line2
            OLED_ShowString(OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_3, "LED control     ");  // show led cmd
            OLED_ShowString(OLED_DISPLAY_COLUMN_START, OLED_DISPLAY_ROW_4, "                ");  // clean line4
        }

        //判断uart thread是否处理完 cmd
        //如果处理完成,把相应发送到 cloud
        if(strlen(uartMsg)!=0 && cmdHaveDone)
        {
			
            {
                err = kNoErr;
                // create json object to format upload data
                send_json_object = json_object_new_object();
                if(NULL == send_json_object)
                {
                    user_log("create json object error!");
                    err = kNoMemoryErr;
                }
                else
                {
                    // add temperature/humidity data into a json oject
                    json_object_object_add(send_json_object, "11", json_object_new_int(13));
                    upload_data = json_object_to_json_string(send_json_object);
                    if(NULL == upload_data)
                    {
                        user_log("create upload data string error!");
                        err = kNoMemoryErr;
                    }
                    else
                    {
                        // check fogcloud connect status
                        if(app_context->appStatus.fogcloudStatus.isCloudConnected)
                        {
                            // upload data string to fogcloud, the seconde param(NULL) means send to defalut topic: '<device_id>/out'
                            MiCOFogCloudMsgSend(app_context, NULL, (unsigned char*)upload_data, strlen(upload_data));
                            user_log("upload data success! \t topic=%s/out",
                                     app_context->appConfig->fogcloudConfig.deviceId);
                            err = kNoErr;
                        }
                    }

                    // free json object memory
                    json_object_put(send_json_object);
                    send_json_object = NULL;
                }
            }
			memset(uartMsg,0x00,sizeof(uartMsg));
			
        }
        else
        {

        }




    }

exit:
    user_log("ERROR: user_main exit with err=%d", err);
    return err;
}
