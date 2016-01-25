/***************************************************************************************
****************************************************************************************
* FILE		: ZigBeeAttributeReport.c
* Description	:
*
* Copyright (c) 2015 by WPI. All Rights Reserved.
*
* History:
* Version		Name       		Date			Description
   0.1		sven	2015/11/30	Initial Version

****************************************************************************************
****************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Debug.h"
#include "MiCOFogCloud.h"
#include "ZigBeeAttributeReport.h"
#include "ZigBeeControlBridge.h"
#include "ZigBeeConstant.h"
#define user_ZBAttr_log(M, ...) custom_log("ZigBee_Attribute", M, ##__VA_ARGS__)
#define user_ZBAttr_log_trace() custom_log_trace("ZigBee_Attribute")


tsZCB_AttributeReports sZCB_Attributes;


/****************************************************************************
* Function	: eZCB_InitAttributeReport
* Description	: Init
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void eZCB_InitAttributeReport()
{
    sZCB_Attributes.psAttribute = NULL;
}



/****************************************************************************
* Function	: eZCB_AddAttributeReport
* Description	: Add Attribute
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_AddAttributeReport(uint16_t u16ShortAddress, uint8_t u8Endpoint, uint16_t u16ClusterID, uint16_t u16AttributeID,teZCL_ZCLAttributeType eType ,tuZcbAttributeData uData, tsZCB_AttributeReport **ppsZCBAttribute)
{
    teZcbStatus eStatus = E_ZCB_OK;
    tsZCB_AttributeReport **my_ppsZCBAttribute = &(sZCB_Attributes.psAttribute);

    while(*my_ppsZCBAttribute)
    {
        if((*my_ppsZCBAttribute)->u16ShortAddress == u16ShortAddress && (*my_ppsZCBAttribute)->u8Endpoint == u8Endpoint && (*my_ppsZCBAttribute)->u16ClusterID == u16AttributeID && (*my_ppsZCBAttribute)->u16AttributeID == u16AttributeID)
        {
            (*my_ppsZCBAttribute)->uData = uData;
            goto done;
        }
        my_ppsZCBAttribute = &((*my_ppsZCBAttribute)->psNext);
    }

    // a new attribute report
    *my_ppsZCBAttribute = malloc(sizeof(tsZCB_AttributeReport));
    if (!(*my_ppsZCBAttribute))
    {
        user_ZBAttr_log("Memory allocation failure allocating node");
        eStatus = E_ZCB_ERROR_NO_MEM;
        goto done;
    }

    memset((*my_ppsZCBAttribute),0x00,sizeof(tsZCB_AttributeReport));

    (*my_ppsZCBAttribute)->u16ShortAddress = u16ShortAddress;
    (*my_ppsZCBAttribute)->u8Endpoint = u8Endpoint;
    (*my_ppsZCBAttribute)->u16ClusterID = u16ClusterID;
    (*my_ppsZCBAttribute)->u16AttributeID = u16AttributeID;
    (*my_ppsZCBAttribute)->eType = eType;
    (*my_ppsZCBAttribute)->uData = uData;
    (*my_ppsZCBAttribute)->psNext = NULL;

    if (ppsZCBAttribute)
    {
        *ppsZCBAttribute = *my_ppsZCBAttribute;
    }

done:
    return eStatus;
}



/****************************************************************************
* Function	: eZCB_RemoveAttributeReport
* Description	: Remove Attribute
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_RemoveAttributeReport(tsZCB_AttributeReport *psZCBAttribute)
{
    teZcbStatus eStatus = E_ZCB_OK;
    tsZCB_AttributeReport **my_ppsZCBAttribute = &(sZCB_Attributes.psAttribute);

    if(!psZCBAttribute)
        return E_ZCB_ERROR;

    while(*my_ppsZCBAttribute)
    {
        if((*my_ppsZCBAttribute) == psZCBAttribute)
        {
            *my_ppsZCBAttribute = (*my_ppsZCBAttribute)->psNext;
            free(psZCBAttribute);
            break;
        }
        my_ppsZCBAttribute = &((*my_ppsZCBAttribute)->psNext);
    }
    return eStatus;
}


/****************************************************************************
* Function	: eZCB_RemoveAllAttributeReport
* Description	: Remove All Attribute
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_RemoveAllAttributeReport()
{
    teZcbStatus eStatus = E_ZCB_OK;
    tsZCB_AttributeReport **my_ppsZCBAttribute = &(sZCB_Attributes.psAttribute);
    while(*my_ppsZCBAttribute)
    {
        eStatus = eZCB_RemoveAttributeReport(*my_ppsZCBAttribute);
        my_ppsZCBAttribute = &((*my_ppsZCBAttribute)->psNext);
    }
    return eStatus;
}


/****************************************************************************
* Function	: psZCB_FindAttribute
* Description	: Find Attribute
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
tsZCB_AttributeReport *psZCB_FindAttribute(uint16_t u16ShortAddress,uint8_t u8Endpoint,uint16_t u16ClusterID,uint16_t u16AttributeID)
{
    //teZcbStatus eStatus = E_ZCB_OK;
    tsZCB_AttributeReport **my_ppsZCBAttribute = &(sZCB_Attributes.psAttribute);

    while(*my_ppsZCBAttribute)
    {
        if( ((*my_ppsZCBAttribute)->u16ShortAddress == u16ShortAddress) && ((*my_ppsZCBAttribute)->u8Endpoint == u8Endpoint) &&((*my_ppsZCBAttribute)->u16ClusterID == u16ClusterID) &&((*my_ppsZCBAttribute)->u16AttributeID == u16AttributeID))
        {
            return *my_ppsZCBAttribute;
        }
        my_ppsZCBAttribute = &((*my_ppsZCBAttribute)->psNext);
    }

    return *my_ppsZCBAttribute;
}


/****************************************************************************
* Function	: eZCB_UpdateAttributeReport
* Description	: Update Attribute
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_UpdateAttributeReport(uint16_t u16ShortAddress,uint8_t u8Endpoint,uint16_t u16ClusterID,uint16_t u16AttributeID,    teZCL_ZCLAttributeType  eType,tuZcbAttributeData uData)
{
    teZcbStatus eStatus = E_ZCB_OK;
    //tsZCB_AttributeReport **my_ppsZCBAttribute = &(sZCB_Attributes.psAttribute);
    tsZCB_AttributeReport *psZCBAttribute = NULL;

    psZCBAttribute = psZCB_FindAttribute(u16ShortAddress,u8Endpoint,u16ClusterID,u16AttributeID);

    if(psZCBAttribute)
    {
        psZCBAttribute->uData = uData;
        return eStatus;
    }
    else
    {
        eStatus = eZCB_AddAttributeReport(u16ShortAddress, u8Endpoint, u16ClusterID, u16AttributeID,eType ,uData, NULL);
        return eStatus;
    }
}


/****************************************************************************
* Function	: eZCB_LogAllAttribute
* Description	: Log All
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void eZCB_LogAllAttribute()
{
    tsZCB_AttributeReport **my_ppsZCBAttribute = &(sZCB_Attributes.psAttribute);
    while(*my_ppsZCBAttribute)
    {
        user_ZBAttr_log("attribute u16ShortAddress %x",(*my_ppsZCBAttribute)->u16ShortAddress);
        user_ZBAttr_log("attribute u8Endpoint %d",(*my_ppsZCBAttribute)->u8Endpoint);
        user_ZBAttr_log("attribute u16ClusterID %d",(*my_ppsZCBAttribute)->u16ClusterID);
        user_ZBAttr_log("attribute u16AttributeID %d",(*my_ppsZCBAttribute)->u16AttributeID);
        user_ZBAttr_log("attribute eType %d",(*my_ppsZCBAttribute)->eType);
        user_ZBAttr_log("attribute uData %d \r\n",(*my_ppsZCBAttribute)->uData.u8Data);
        my_ppsZCBAttribute = &((*my_ppsZCBAttribute)->psNext);
    }
}

void testAttribute()
{
    uint8_t i;
    tsZCB_AttributeReport *psZCBAttribute;
    uint16_t u16ShortAddress = 0x0001;
    uint8_t u8Endpoint = 1;
    uint16_t u16ClusterID = 0x0101;
    uint16_t u16AttributeID = 0x0101;
    teZCL_ZCLAttributeType eType = E_ZCL_UINT8;

    tuZcbAttributeData uData;
    uData.u8Data = 0x55;

    eZCB_InitAttributeReport();

    for(i=0; i<10; i++)
    {
        eZCB_AddAttributeReport(u16ShortAddress, u8Endpoint, u16ClusterID, u16AttributeID, eType, uData, NULL);
        u16ShortAddress++;
        u8Endpoint++;
        u16ClusterID++;
        u16AttributeID++;
    }
    eZCB_LogAllAttribute();

    u16ShortAddress = 0x0001;
    u8Endpoint = 1;
    u16ClusterID = 0x0101;
    u16AttributeID = 0x0101;
    eType = E_ZCL_UINT8;

    for(i=0; i<5; i++)
    {
        psZCBAttribute = psZCB_FindAttribute(u16ShortAddress, u8Endpoint, u16ClusterID, u16AttributeID);
        eZCB_RemoveAttributeReport(psZCBAttribute);
        u16ShortAddress += 2;
        u8Endpoint += 2;
        u16ClusterID += 2;
        u16AttributeID += 2;
    }
    eZCB_LogAllAttribute();

}




/****************************************************************************
* Function	: eZCB_ReportAttributeToCloud
* Description	: Report Attribute to cloud
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_ReportAttributeToCloud(tsZCB_AttributeReport *psZCBAttribute,app_context_t * const app_context )
{
    struct json_object* json_object = NULL;
    struct json_object* send_json_objects = NULL;
    struct json_object* actual_send_json_objects = NULL;
    const char *upload_data = NULL;

    actual_send_json_objects = json_object_new_object();
    if(NULL == actual_send_json_objects)
    {
        user_ZBAttr_log("create json object error!");
        //err = kNoMemoryErr;
    }
    else
    {
        send_json_objects = json_object_new_object();
        if(NULL == send_json_objects)
        {
            user_ZBAttr_log("create json object error!");
            //err = kNoMemoryErr;
        }
        else
        {
            json_object = json_object_new_object();
            if(NULL == json_object)
            {
                user_ZBAttr_log("create json object error!");
                //err = kNoMemoryErr;
            }
            else
            {
                /*
                uint16_t                u16ShortAddress;
                uint8_t                 u8Endpoint;
                uint16_t                u16ClusterID;
                uint16_t                u16AttributeID;
                teZCL_ZCLAttributeType  eType;
                tuZcbAttributeData      uData;
                */
                // add temperature/humidity data into a json oject  增加数据到 json 对象
                json_object_object_add(json_object, "Zigbee_shortAddress", json_object_new_int(psZCBAttribute->u16ShortAddress));
                json_object_object_add(json_object, "Zigbee_endpointID", json_object_new_int(psZCBAttribute->u8Endpoint));
                json_object_object_add(json_object, "Zigbee_clusterID", json_object_new_int(psZCBAttribute->u16ClusterID));
                json_object_object_add(json_object, "Zigbee_attributeID", json_object_new_int(psZCBAttribute->u16AttributeID));
                json_object_object_add(json_object, "Zigbee_attributeType", json_object_new_int(psZCBAttribute->eType));
                switch(psZCBAttribute->eType)
                {
                    case(E_ZCL_GINT8):
                    case(E_ZCL_UINT8):
                    case(E_ZCL_INT8):
                    case(E_ZCL_ENUM8):
                    case(E_ZCL_BMAP8):
                    case(E_ZCL_BOOL):
                    case(E_ZCL_OSTRING):
                    case(E_ZCL_CSTRING):
                        json_object_object_add(json_object, "Zigbee_attributeData", json_object_new_int(psZCBAttribute->uData.u8Data));

                        break;

                    case(E_ZCL_LOSTRING):
                    case(E_ZCL_LCSTRING):
                    case(E_ZCL_STRUCT):
                    case(E_ZCL_INT16):
                    case(E_ZCL_UINT16):
                    case(E_ZCL_ENUM16):
                    case(E_ZCL_CLUSTER_ID):
                    case(E_ZCL_ATTRIBUTE_ID):
                        json_object_object_add(json_object, "Zigbee_attributeData", json_object_new_int(psZCBAttribute->uData.u16Data));
                        break;

                    case(E_ZCL_UINT24):
                    case(E_ZCL_UINT32):
                    case(E_ZCL_TOD):
                    case(E_ZCL_DATE):
                    case(E_ZCL_UTCT):
                    case(E_ZCL_BACNET_OID):
                        json_object_object_add(json_object, "Zigbee_attributeData", json_object_new_int(psZCBAttribute->uData.u32Data));

                        break;

                    case(E_ZCL_UINT40):
                    case(E_ZCL_UINT48):
                    case(E_ZCL_UINT56):
                    case(E_ZCL_UINT64):
                    case(E_ZCL_IEEE_ADDR):
                        json_object_object_add(json_object, "Zigbee_attributeData", json_object_new_int(psZCBAttribute->uData.u64Data));

                        break;

                    default:
                        user_ZBAttr_log("Unknown attribute data type (%d) ",psZCBAttribute->eType);
                        break;
                }

                json_object_object_add(send_json_objects, "dd", json_object);
            }
            json_object = NULL;

            json_object_object_add(actual_send_json_objects, "attributeRepoet", send_json_objects);
			
            // json object convert to json string
            upload_data = json_object_to_json_string(actual_send_json_objects);

            if(NULL == upload_data)
            {
                user_ZBAttr_log("create upload data string error!");
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
                    user_ZBAttr_log("upload data success! \t topic=%s/out",
                             app_context->appConfig->fogcloudConfig.deviceId);
                }
            }

            // free json object memory
            json_object_put(actual_send_json_objects);
            send_json_objects = NULL;
        }
    }
    return E_ZCB_OK;
}



