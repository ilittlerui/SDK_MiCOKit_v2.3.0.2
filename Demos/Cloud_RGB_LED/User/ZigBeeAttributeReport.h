/***************************************************************************************
****************************************************************************************
* FILE		: ZigBeeAttributeReport.h
* Description	: 
*			  
* Copyright (c) 2015 by WPI. All Rights Reserved.
* 
* History:
* Version		Name       		Date			Description
   0.1		sven	2015/11/30	Initial Version
   
****************************************************************************************
****************************************************************************************/
#ifndef _ZIGBEEATTRIBUTESREPORT_H_
#define _ZIGBEEATTRIBUTESREPORT_H_

#include "ZigBeeConstant.h"
#include "ZigBeeControlBridge.h"
#include "MICOAppDefine.h"


typedef struct _tsZCB_AttributeReport
{
    uint16_t                u16ShortAddress;
    uint8_t                 u8Endpoint;
    uint16_t                u16ClusterID;
    uint16_t                u16AttributeID;
    teZCL_ZCLAttributeType  eType;
    tuZcbAttributeData      uData;
    struct _tsZCB_AttributeReport *psNext;
} tsZCB_AttributeReport;

/** Stucture for the Zigbee attribute Report Node */
typedef struct
{
    mico_mutex_t            sLock;              /**< Lock for the attribute list */

    tsZCB_AttributeReport              *psAttribute;             /**< Linked list of attribute.*/
} tsZCB_AttributeReports;

void eZCB_InitAttributeReport();
teZcbStatus eZCB_AddAttributeReport(uint16_t u16ShortAddress, uint8_t u8Endpoint, uint16_t u16ClusterID, uint16_t u16AttributeID,teZCL_ZCLAttributeType eType ,tuZcbAttributeData uData, tsZCB_AttributeReport **ppsZCBAttribute);
teZcbStatus eZCB_RemoveAttributeReport(tsZCB_AttributeReport *psZCBAttribute);
teZcbStatus eZCB_RemoveAllAttributeReport();
tsZCB_AttributeReport *psZCB_FindAttribute(uint16_t u16ShortAddress,uint8_t u8Endpoint,uint16_t u16ClusterID,uint16_t u16AttributeID);
teZcbStatus eZCB_UpdateAttributeReport(uint16_t u16ShortAddress,uint8_t u8Endpoint,uint16_t u16ClusterID,uint16_t u16AttributeID,    teZCL_ZCLAttributeType  eType,tuZcbAttributeData uData);
void eZCB_LogAllAttribute();
void testAttribute();
teZcbStatus eZCB_ReportAttributeToCloud(tsZCB_AttributeReport *psZCBAttribute,app_context_t * const app_context );
#endif /*_ZIGBEEATTRIBUTESREPORT_H_*/

