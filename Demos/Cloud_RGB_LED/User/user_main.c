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

/* User defined debug log functions
 * Add your own tag like: 'USER', the tag will be added at the beginning of a log
 * in MICO debug uart, when you call this function.
 */
#define user_log(M, ...) custom_log("USER", M, ##__VA_ARGS__)
#define user_log_trace() custom_log_trace("USER")

#pragma pack(show)

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
    user_log("User uart Init");
    require_noerr_action( err, exit, user_log("ERROR: user_uartInit err = %d.", err) );

    eZCB_Init();			//ZigBee ControlBridge Init		初始化zigbee协调器
    user_log("ZCB Init");

    uint8_t cloudMsg[128];
 
    teZcbStatus eStatus;
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
                        memset(cloudMsg,0x0,sizeof(cloudMsg));
                        strncpy((char*)cloudMsg, (char*)json_object_get_string(val), json_object_get_string_len(val));

                        if(strncmp((char*)"init",(char*)cloudMsg,json_object_get_string_len(val))==0)
                        {
                            user_log("init cmd");
                            //eZCB_ConfigureControlBridge();
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
                        else if(strcmp((char*)"device",(char*)(recv_msg->data + recv_msg->topic_len))==0)
                        {
                            user_log("get device");
                        }
                        else if(strcmp((char*)"permit",(char*)(recv_msg->data + recv_msg->topic_len))==0)
                        {
                            user_log("permit device join");
                            eZCB_SetPermitJoining(60);
                        }
                        else
                        {
                            user_log("err cmd");
                        }

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
    }

exit:
    user_log("ERROR: user_main exit with err=%d", err);
    return err;
}
