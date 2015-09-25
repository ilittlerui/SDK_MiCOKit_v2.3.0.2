
/***************************************************************************************
****************************************************************************************
* FILE		: ZigbeeControlBridge.c
* Description	:
*
* Copyright (c) 2015 by XXX. All Rights Reserved.
*
* History:
* Version		Name       		Date			Description
0.1		XXX	2015/07/01	Initial Version

****************************************************************************************
****************************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MICO.h"
#include "MICOAppDefine.h"
#include "ZigBeeControlBridge.h"
#include "ZigBeeNetwork.h"
#include "ZigBeeConstant.h"
#include "ZigBeeSerialLink.h"
#include "ZigBeePDM.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_ZCB 0


#define user_controlbridge_log(M, ...) custom_log("ControlBridge", M, ##__VA_ARGS__)
#define user_controlbridge_log_trace() custom_log_trace("ControlBridge")

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/



/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/** Mode */
teStartMode      eStartMode          = CONFIG_DEFAULT_START_MODE;
teChannel        eChannel            = CONFIG_DEFAULT_CHANNEL;
uint64_t         u64PanID            = CONFIG_DEFAULT_PANID;

/* Network parameters in use */
teChannel        eChannelInUse       = 0;
uint64_t         u64PanIDInUse       = 0;
uint16_t         u16PanIDInUse       = 0;

//tsUtilsQueue    sZcbEventQueue;

/* APS Ack enabled by default */
int              bZCB_EnableAPSAck  = 1;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/


/** Firmware version of the connected device */
uint32_t u32ZCB_SoftwareVersion = 0;



/** Initialise control bridge connected to serial port */

/****************************************************************************
* Function	: eZCB_Init
* Description	: Init ZigBeeControlBridge
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_Init(app_context_t * const app_context)
{
    if (eSL_Init() != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }

    /* Create the event queue for the control bridge. The queue will not block if space is not available */
    //if (eUtils_QueueCreate(&sZcbEventQueue, 100, UTILS_QUEUE_NONBLOCK_INPUT) != E_UTILS_OK)
    //{
    //    user_controlbridge_log("Error initialising event queue");
    //    return E_ZCB_ERROR;
    //}
    //初始化 ZigBee Network
    memset(&sZCB_Network, 0, sizeof(sZCB_Network));
    mico_rtos_init_mutex(&sZCB_Network.sLock);
    mico_rtos_init_mutex(&sZCB_Network.sNodes.sLock);

    // Get the PDM going so that the node can get the information it needs.
    //ePDM_Init((mico_Context_t*)app_context->mico_context);
    user_controlbridge_log("PDM init");

    return E_ZCB_OK;
}



/****************************************************************************
* Function	: eZCB_Finish
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_Finish(void)
{
    ePDM_Destory();
    eSL_Destroy();

    //if (eUtils_QueueDestroy(&sZcbEventQueue) != E_UTILS_OK)
    //{
    //   daemon_log(LOG_ERR, "Error destroying event queue");
    //}

    while (sZCB_Network.sNodes.psNext)
    {
        eZCB_RemoveNode(sZCB_Network.sNodes.psNext);
    }
    eZCB_RemoveNode(&sZCB_Network.sNodes);
    mico_rtos_deinit_mutex(&sZCB_Network.sLock);

    return E_ZCB_OK;
}



/****************************************************************************
* Function	: eZCB_EstablishComms
* Description	: ZCB Establish Comms
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_EstablishComms(void)
{
    if (eSL_SendMessage(E_SL_MSG_GET_VERSION, 0, NULL, NULL) == E_SL_OK)
    {
        uint16_t u16Length;
        uint32_t  *u32Version;

        /* Wait 300ms for the versions message to arrive */
        if (eSL_MessageWait(E_SL_MSG_VERSION_LIST, 300, &u16Length, (void**)&u32Version) == E_SL_OK)
        {
            u32ZCB_SoftwareVersion = ntohl(*u32Version);
            user_controlbridge_log("Connected to control bridge version 0x%08x", u32ZCB_SoftwareVersion);
            if(u32Version != NULL)
                free(u32Version);

            user_controlbridge_log("Reset control bridge\n");
            if (eSL_SendMessage(E_SL_MSG_RESET, 0, NULL, NULL) != E_SL_OK)
            {
                return E_ZCB_COMMS_FAILED;
            }
            return E_ZCB_OK;
        }
    }

    return E_ZCB_COMMS_FAILED;
}



/****************************************************************************
* Function	: eZCB_FactoryNew
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_FactoryNew(void)
{
    teSL_Status eStatus;
    user_controlbridge_log("Factory resetting control bridge");

    if ((eStatus = eSL_SendMessage(E_SL_MSG_ERASE_PERSISTENT_DATA, 0, NULL, NULL)) != E_SL_OK)//Send Message
    {
        user_controlbridge_log("eStatus:%d",eStatus);
        if (eStatus == E_SL_NOMESSAGE)
        {
            /* The erase persistent data command could take a while */
            uint16_t u16Length;
            tsSL_Msg_Status *psStatus = NULL;

            eStatus = eSL_MessageWait(E_SL_MSG_STATUS, 500, &u16Length, (void**)&psStatus);

            if (eStatus == E_SL_OK)
            {
                eStatus = psStatus->eStatus;
                if(psStatus != NULL)
                    free(psStatus);
            }
            else

            {
                return E_ZCB_COMMS_FAILED;
            }
        }
        else
        {
            return E_ZCB_COMMS_FAILED;
        }
    }
    /* Wait for it to erase itself */
    mico_thread_sleep(1);

    user_controlbridge_log("Reset control bridge");
    if (eSL_SendMessage(E_SL_MSG_RESET, 0, NULL, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}



/****************************************************************************
* Function	: eZCB_SetExtendedPANID
* Description	: 设置 Extended PANID
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_SetExtendedPANID(uint64_t u64PanID)
{
    //u64PanID = htobe64(u64PanID);
    u64PanID = hton64(u64PanID);
    if (eSL_SendMessage(E_SL_MSG_SET_EXT_PANID, sizeof(uint64_t), &u64PanID, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}


/****************************************************************************
* Function	: eZCB_SetChannelMask
* Description	: 设置信道掩码
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_SetChannelMask(uint32_t u32ChannelMask)
{
    user_controlbridge_log("Setting channel mask: 0x%08X", u32ChannelMask);
    u32ChannelMask = htonl(u32ChannelMask);

    if (eSL_SendMessage(E_SL_MSG_SET_CHANNELMASK, sizeof(uint32_t), &u32ChannelMask, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}


/****************************************************************************
* Function	: eZCB_SetInitialSecurity
* Description	:  设置 security key
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_SetInitialSecurity(uint8_t u8State, uint8_t u8Sequence, uint8_t u8Type, uint8_t *pu8Key)
{
    uint8_t au8Buffer[256];
    uint32_t u32Position = 0;

    au8Buffer[u32Position] = u8State;
    u32Position += sizeof(uint8_t);

    au8Buffer[u32Position] = u8Sequence;
    u32Position += sizeof(uint8_t);

    au8Buffer[u32Position] = u8Type;
    u32Position += sizeof(uint8_t);

    switch (u8Type)
    {
        case (1):
            memcpy(&au8Buffer[u32Position], pu8Key, 16);
            u32Position += 16;
            break;
        default:
            user_controlbridge_log("Uknown key type %d\n", u8Type);
            return E_ZCB_ERROR;
    }

    if (eSL_SendMessage(E_SL_MSG_SET_SECURITY, u32Position, au8Buffer, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}


/****************************************************************************
* Function	: eZCB_SetDeviceType
* Description	: Set Device Type
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_SetDeviceType(teModuleMode eModuleMode)
{
    uint8_t u8ModuleMode = eModuleMode;

    user_controlbridge_log("Writing Module: Set Device Type: %d", eModuleMode);


    if (eSL_SendMessage(E_SL_MSG_SET_DEVICETYPE, sizeof(uint8_t), &u8ModuleMode, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}


/****************************************************************************
* Function	: eZCB_StartNetwork
* Description	: Start Network
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_StartNetwork(void)
{
    user_controlbridge_log("Start network");
    if (eSL_SendMessage(E_SL_MSG_START_NETWORK, 0, NULL, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}


/****************************************************************************
* Function	: eZCB_SetPermitJoining
* Description	: Set Zigbee Permit Join
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_SetPermitJoining(uint8_t u8Interval)
{
#pragma pack(1)
    struct _PermitJoiningMessage
    {
        uint16_t    u16TargetAddress;
        uint8_t     u8Interval;
        uint8_t     u8TCSignificance;
    } sPermitJoiningMessage;
#pragma pack()
    user_controlbridge_log("Permit joining (%d)", u8Interval);

    sPermitJoiningMessage.u16TargetAddress  = htons(E_ZB_BROADCAST_ADDRESS_ROUTERS);
    sPermitJoiningMessage.u8Interval        = u8Interval;
    sPermitJoiningMessage.u8TCSignificance  = 0;

    if (eSL_SendMessage(E_SL_MSG_PERMIT_JOINING_REQUEST, sizeof(struct _PermitJoiningMessage), &sPermitJoiningMessage, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}


/** Get permit joining status of the control bridge */
/****************************************************************************
* Function	: eZCB_GetPermitJoining
* Description	: Get Permit Join Status
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_GetPermitJoining(uint8_t *pu8Status)
{
#pragma pack(1)
    struct _GetPermitJoiningMessage
    {
        uint8_t     u8Status;
    }*psGetPermitJoiningResponse;
    uint16_t u16Length;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
#pragma pack()
    if (eSL_SendMessage(E_SL_MSG_GET_PERMIT_JOIN, 0, NULL, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }

    /* Wait 1 second for the message to arrive */
    if (eSL_MessageWait(E_SL_MSG_GET_PERMIT_JOIN_RESPONSE, 1000, &u16Length, (void**)&psGetPermitJoiningResponse) != E_SL_OK)
    {

        user_controlbridge_log("No response to permit joining request");

        goto done;
    }

    if (pu8Status)
    {
        *pu8Status = psGetPermitJoiningResponse->u8Status;
    }

    user_controlbridge_log("Permit joining Status: %d", psGetPermitJoiningResponse->u8Status);

    free(psGetPermitJoiningResponse);
    eStatus = E_ZCB_OK;
done:
    return eStatus;
}


/****************************************************************************
* Function	: eZCB_SetWhitelistEnabled
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_SetWhitelistEnabled(uint8_t bEnable)
{
    uint8_t u8Enable = bEnable ? 1 : 0;

    user_controlbridge_log("%s whitelisting\n", bEnable ? "Enable" : "Disable");
    if (eSL_SendMessage(E_SL_MSG_NETWORK_WHITELIST_ENABLE, 1, &u8Enable, NULL) != E_SL_OK)
    {
        user_controlbridge_log("Failed\n");
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}


/****************************************************************************
* Function	: eZCB_AuthenticateDevice
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_AuthenticateDevice(uint64_t u64IEEEAddress, uint8_t *pau8LinkKey,
                                    uint8_t *pau8NetworkKey, uint8_t *pau8MIC,
                                    uint64_t *pu64TrustCenterAddress, uint8_t *pu8KeySequenceNumber)
{
#pragma pack(1)
    struct _AuthenticateRequest
    {
        uint64_t    u64IEEEAddress;
        uint8_t     au8LinkKey[16];
    } sAuthenticateRequest;
#pragma pack()

#pragma pack(1)
    struct _AuthenticateResponse
    {
        uint64_t    u64IEEEAddress;
        uint8_t     au8NetworkKey[16];
        uint8_t     au8MIC[4];
        uint64_t    u64TrustCenterAddress;
        uint8_t     u8KeySequenceNumber;
        uint8_t     u8Channel;
        uint16_t    u16PanID;
        uint64_t    u64PanID;
    }*psAuthenticateResponse = NULL;
#pragma pack()
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    //sAuthenticateRequest.u64IEEEAddress = htobe64(u64IEEEAddress);
    sAuthenticateRequest.u64IEEEAddress = hton64(u64IEEEAddress);
    memcpy(sAuthenticateRequest.au8LinkKey, pau8LinkKey, 16);

    if (eSL_SendMessage(E_SL_MSG_AUTHENTICATE_DEVICE_REQUEST, sizeof(struct _AuthenticateRequest), &sAuthenticateRequest, &u8SequenceNo) == E_SL_OK)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_AUTHENTICATE_DEVICE_RESPONSE, 1000, &u16Length, (void**)&psAuthenticateResponse) != E_SL_OK)
        {
            //if (verbosity > LOG_INFO)
            //{
            //    daemon_log(LOG_DEBUG, "No response to authenticate request");
            //}
        }
        else
        {
            user_controlbridge_log("Got authentication data for device 0x%016llX\n", (unsigned long long int)u64IEEEAddress);

            psAuthenticateResponse->u64TrustCenterAddress = hton64(psAuthenticateResponse->u64TrustCenterAddress);
            psAuthenticateResponse->u16PanID = ntohs(psAuthenticateResponse->u16PanID);
            psAuthenticateResponse->u64PanID = ntoh64(psAuthenticateResponse->u64PanID);

            user_controlbridge_log("Trust center address: 0x%016llX\n", (unsigned long long int)psAuthenticateResponse->u64TrustCenterAddress);
            user_controlbridge_log("Key sequence number: %02d\n", psAuthenticateResponse->u8KeySequenceNumber);
            user_controlbridge_log("Channel: %02d\n", psAuthenticateResponse->u8Channel);
            user_controlbridge_log("Short PAN: 0x%04X\n", psAuthenticateResponse->u16PanID);
            user_controlbridge_log("Extended PAN: 0x%016llX\n", (unsigned long long int)psAuthenticateResponse->u64PanID);

            memcpy(pau8NetworkKey, psAuthenticateResponse->au8NetworkKey, 16);
            memcpy(pau8MIC, psAuthenticateResponse->au8MIC, 4);
            memcpy(pu64TrustCenterAddress, &psAuthenticateResponse->u64TrustCenterAddress, 8);
            memcpy(pu8KeySequenceNumber, &psAuthenticateResponse->u8KeySequenceNumber, 1);

            eStatus = E_ZCB_OK;
        }
    }

    if(psAuthenticateResponse != NULL)
        free(psAuthenticateResponse);
    return eStatus;
}


/** Initiate Touchlink */
/****************************************************************************
* Function	: eZCB_ZLL_Touchlink
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_ZLL_Touchlink(void)
{
    user_controlbridge_log("Initiate Touchlink\n");
    if (eSL_SendMessage(E_SL_MSG_INITIATE_TOUCHLINK, 0, NULL, NULL) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }
    return E_ZCB_OK;
}



/** Initiate Match descriptor request */
teZcbStatus eZCB_MatchDescriptorRequest(uint16_t u16TargetAddress, uint16_t u16ProfileID,
                                        uint8_t u8NumInputClusters, uint16_t *pau16InputClusters,
                                        uint8_t u8NumOutputClusters, uint16_t *pau16OutputClusters,
                                        uint8_t *pu8SequenceNo)
{
    uint8_t au8Buffer[256];
    uint32_t u32Position = 0;
    int i;

    user_controlbridge_log("Send Match Desciptor request for profile ID 0x%04X to 0x%04X", u16ProfileID, u16TargetAddress);

    u16TargetAddress = htons(u16TargetAddress);
    memcpy(&au8Buffer[u32Position], &u16TargetAddress, sizeof(uint16_t));
    u32Position += sizeof(uint16_t);

    u16ProfileID = htons(u16ProfileID);
    memcpy(&au8Buffer[u32Position], &u16ProfileID, sizeof(uint16_t));
    u32Position += sizeof(uint16_t);

    au8Buffer[u32Position] = u8NumInputClusters;
    u32Position++;

    user_controlbridge_log("  Input Cluster List:");

    for (i = 0; i < u8NumInputClusters; i++)
    {
        uint16_t u16ClusterID = htons(pau16InputClusters[i]);
        user_controlbridge_log("    0x%04X", pau16InputClusters[i]);
        memcpy(&au8Buffer[u32Position], &u16ClusterID , sizeof(uint16_t));
        u32Position += sizeof(uint16_t);
    }

    user_controlbridge_log("  Output Cluster List:");

    au8Buffer[u32Position] = u8NumOutputClusters;
    u32Position++;

    for (i = 0; i < u8NumOutputClusters; i++)
    {
        uint16_t u16ClusterID = htons(pau16OutputClusters[i] );
        user_controlbridge_log("    0x%04X", pau16OutputClusters[i]);
        memcpy(&au8Buffer[u32Position], &u16ClusterID , sizeof(uint16_t));
        u32Position += sizeof(uint16_t);
    }

    if (eSL_SendMessage(E_SL_MSG_MATCH_DESCRIPTOR_REQUEST, u32Position, au8Buffer, pu8SequenceNo) != E_SL_OK)
    {
        return E_ZCB_COMMS_FAILED;
    }

    return E_ZCB_OK;
}


/****************************************************************************
* Function	: eZCB_LeaveRequest
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_LeaveRequest(tsZCB_Node *psZCBNode)
{
#pragma pack(1)
    struct _ManagementLeaveRequest
    {
        uint16_t    u16TargetAddress;
        uint64_t    u64IEEEAddress;
        uint8_t     bRemoveChildren;
        uint8_t     bRejoin;
    } sManagementLeaveRequest;
#pragma pack()

#pragma pack(1)
    struct _ManagementLeaveResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
    }*sManagementLeaveResponse = NULL;
#pragma pack()
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    user_controlbridge_log("\n\n\nRequesting node 0x%04X to leave network\n", psZCBNode->u16ShortAddress);

    sManagementLeaveRequest.u16TargetAddress    = htons(psZCBNode->u16ShortAddress);
    sManagementLeaveRequest.u64IEEEAddress      = 0; // Ignored
    sManagementLeaveRequest.bRemoveChildren     = 0;
    sManagementLeaveRequest.bRejoin             = 0;

    if (eSL_SendMessage(E_SL_MSG_NETWORK_REMOVE_DEVICE, sizeof(struct _ManagementLeaveRequest), &sManagementLeaveRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the leave confirmation message to arrive */
        if (eSL_MessageWait(E_SL_MSG_LEAVE_CONFIRMATION, 1000, &u16Length, (void**)&sManagementLeaveResponse) != E_SL_OK)
        {
            user_controlbridge_log("No response to management leave request");
            goto done;
        }

        if (u8SequenceNo == sManagementLeaveResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("leave response sequence number received 0x%02X does not match that sent 0x%02X\n", sManagementLeaveResponse->u8SequenceNo, u8SequenceNo);
            if(sManagementLeaveResponse != NULL)
                free(sManagementLeaveResponse);
        }
    }

    user_controlbridge_log("Request Node 0x%04X to leave status: %d\n", psZCBNode->u16ShortAddress, sManagementLeaveResponse->u8Status);

    eStatus = sManagementLeaveResponse->u8Status;

done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    if(sManagementLeaveResponse != NULL)
        free(sManagementLeaveResponse);
    return eStatus;
}



/****************************************************************************
* Function	: eZCB_NeighbourTableRequest
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_NeighbourTableRequest(tsZCB_Node *psZCBNode)
{
#pragma pack(1)
    struct _ManagementLQIRequest
    {
        uint16_t    u16TargetAddress;
        uint8_t     u8StartIndex;
    } sManagementLQIRequest;
#pragma pack()

#pragma pack(1)
    struct _ManagementLQIResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint8_t     u8NeighbourTableSize;
        uint8_t     u8TableEntries;
        uint8_t     u8StartIndex;
#pragma pack(1)
        struct
        {
            uint16_t    u16ShortAddress;
            uint64_t    u64PanID;
            uint64_t    u64IEEEAddress;
            uint8_t     u8Depth;
            uint8_t     u8LQI;
#pragma pack(1)
            struct
            {
                unsigned    uDeviceType : 2;
                unsigned    uPermitJoining: 2;
                unsigned    uRelationship : 2;
                unsigned    uMacCapability : 2;
            } sBitmap;
#pragma pack()
        } asNeighbours[255];
#pragma pack()
    }*psManagementLQIResponse = NULL;
#pragma pack()
    uint16_t u16ShortAddress;
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    int i;

    u16ShortAddress = psZCBNode->u16ShortAddress;

    /* Unlock the node during this process, because it can take time, and we don't want to be holding a node lock when
     * attempting to lock the list of nodes - that leads to deadlocks with the JIP server thread. */
    mico_rtos_unlock_mutex(&psZCBNode->sLock);

    sManagementLQIRequest.u16TargetAddress = htons(u16ShortAddress);
    sManagementLQIRequest.u8StartIndex      = psZCBNode->u8LastNeighbourTableIndex;

    user_controlbridge_log("Send management LQI request to 0x%04X for entries starting at %d\n", u16ShortAddress, sManagementLQIRequest.u8StartIndex);

    if (eSL_SendMessage(E_SL_MSG_MANAGEMENT_LQI_REQUEST, sizeof(struct _ManagementLQIRequest), &sManagementLQIRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_MANAGEMENT_LQI_RESPONSE, 1000, &u16Length, (void**)&psManagementLQIResponse) != E_SL_OK)
        {
            user_controlbridge_log("No response to management LQI request");
            goto done;
        }
        else if (u8SequenceNo == psManagementLQIResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("IEEE Address sequence number received 0x%02X does not match that sent 0x%02X\n", psManagementLQIResponse->u8SequenceNo, u8SequenceNo);
            if(psManagementLQIResponse != NULL)
                free(psManagementLQIResponse);
        }
    }

    user_controlbridge_log("Received management LQI response. Table size: %d, Entry count: %d, start index: %d\n",
                           psManagementLQIResponse->u8NeighbourTableSize,
                           psManagementLQIResponse->u8TableEntries,
                           psManagementLQIResponse->u8StartIndex);

    for (i = 0; i < psManagementLQIResponse->u8TableEntries; i++)
    {
        tsZCB_Node *psZCBNode;
        tsZcbEvent *psEvent;

        psManagementLQIResponse->asNeighbours[i].u16ShortAddress    = ntohs(psManagementLQIResponse->asNeighbours[i].u16ShortAddress);
        //psManagementLQIResponse->asNeighbours[i].u64PanID           = be64toh(psManagementLQIResponse->asNeighbours[i].u64PanID);
        psManagementLQIResponse->asNeighbours[i].u64PanID           = ntoh64(psManagementLQIResponse->asNeighbours[i].u64PanID);
        //psManagementLQIResponse->asNeighbours[i].u64IEEEAddress     = be64toh(psManagementLQIResponse->asNeighbours[i].u64IEEEAddress);
        psManagementLQIResponse->asNeighbours[i].u64IEEEAddress     = ntoh64(psManagementLQIResponse->asNeighbours[i].u64IEEEAddress);
        if ((psManagementLQIResponse->asNeighbours[i].u16ShortAddress >= 0xFFFA) ||
                (psManagementLQIResponse->asNeighbours[i].u64IEEEAddress  == 0))
        {
            /* Illegal short / IEEE address */
            continue;
        }

        user_controlbridge_log("  Entry %02d: Short Address 0x%04X, PAN ID: 0x%016llX, IEEE Address: 0x%016llX\n", i,
                               psManagementLQIResponse->asNeighbours[i].u16ShortAddress,
                               (unsigned long long int)psManagementLQIResponse->asNeighbours[i].u64PanID,
                               (unsigned long long int)psManagementLQIResponse->asNeighbours[i].u64IEEEAddress);

        user_controlbridge_log("    Type: %d, Permit Joining: %d, Relationship: %d, RxOnWhenIdle: %d\n",
                               psManagementLQIResponse->asNeighbours[i].sBitmap.uDeviceType,
                               psManagementLQIResponse->asNeighbours[i].sBitmap.uPermitJoining,
                               psManagementLQIResponse->asNeighbours[i].sBitmap.uRelationship,
                               psManagementLQIResponse->asNeighbours[i].sBitmap.uMacCapability);

        user_controlbridge_log("    Depth: %d, LQI: %d\n",
                               psManagementLQIResponse->asNeighbours[i].u8Depth,
                               psManagementLQIResponse->asNeighbours[i].u8LQI);

        psZCBNode = psZCB_FindNodeShortAddress(psManagementLQIResponse->asNeighbours[i].u16ShortAddress);

        if (psZCBNode)
        {
            mico_rtos_unlock_mutex(&psZCBNode->sLock);
        }
        else
        {
            user_controlbridge_log("New Node 0x%04X in neighbour table\n", psManagementLQIResponse->asNeighbours[i].u16ShortAddress);

            if ((eStatus = eZCB_AddNode(psManagementLQIResponse->asNeighbours[i].u16ShortAddress,
                                        psManagementLQIResponse->asNeighbours[i].u64IEEEAddress,
                                        0x0000, psManagementLQIResponse->asNeighbours[i].sBitmap.uMacCapability ? E_ZB_MAC_CAPABILITY_RXON_WHEN_IDLE : 0, NULL)) != E_ZCB_OK)
            {
                user_controlbridge_log("Error adding node to network\n");
                break;
            }
        }

        psEvent = malloc(sizeof(tsZcbEvent));
        if (!psEvent)
        {
            user_controlbridge_log("Memory allocation failure allocating event");
            eStatus = E_ZCB_ERROR_NO_MEM;
            goto done;
        }

        psEvent->eEvent                                 = E_ZCB_EVENT_DEVICE_ANNOUNCE;
        psEvent->uData.sDeviceAnnounce.u16ShortAddress  = psManagementLQIResponse->asNeighbours[i].u16ShortAddress;

        teZcbStatus status ;
        status = eZCB_HandleZcbEvent(psEvent);
        if(status != E_ZCB_OK)
            user_controlbridge_log("Handle Zcb Event err");
        //if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != E_UTILS_OK)
        //{
        //   user_controlbridge_log("Error queue'ing event\n");
        //    free(psEvent);
        //}
        //else
        //{
        //    user_controlbridge_log("Device join queued\n");
        //}
    }

    if (psManagementLQIResponse->u8TableEntries > 0)
    {
        // We got some entries, so next time request the entries after these.
        psZCBNode->u8LastNeighbourTableIndex += psManagementLQIResponse->u8TableEntries;
    }
    else
    {
        // No more valid entries.
        psZCBNode->u8LastNeighbourTableIndex = 0;
    }

    eStatus = E_ZCB_OK;
done:
    psZCBNode = psZCB_FindNodeShortAddress(u16ShortAddress);
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    if(psManagementLQIResponse != NULL)
        free(psManagementLQIResponse);
    return eStatus;
}



/****************************************************************************
* Function	: eZCB_IEEEAddressRequest
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_IEEEAddressRequest(tsZCB_Node *psZCBNode)
{
#pragma pack(1)
    struct _IEEEAddressRequest
    {
        uint16_t    u16TargetAddress;
        uint16_t    u16ShortAddress;
        uint8_t     u8RequestType;
        uint8_t     u8StartIndex;
    } sIEEEAddressRequest;
#pragma pack()
#pragma pack(1)
    struct _IEEEAddressResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint64_t    u64IEEEAddress;
        uint16_t    u16ShortAddress;
        uint8_t     u8NumAssociatedDevices;
        uint8_t     u8StartIndex;
        uint16_t    au16DeviceList[255];
    }*psIEEEAddressResponse = NULL;
#pragma pack()
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    user_controlbridge_log("Send IEEE Address request to 0x%04X\n", psZCBNode->u16ShortAddress);

    sIEEEAddressRequest.u16TargetAddress    = htons(psZCBNode->u16ShortAddress);
    sIEEEAddressRequest.u16ShortAddress     = htons(psZCBNode->u16ShortAddress);
    sIEEEAddressRequest.u8RequestType       = 0;
    sIEEEAddressRequest.u8StartIndex        = 0;

    if (eSL_SendMessage(E_SL_MSG_IEEE_ADDRESS_REQUEST, sizeof(struct _IEEEAddressRequest), &sIEEEAddressRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_IEEE_ADDRESS_RESPONSE, 5000, &u16Length, (void**)&psIEEEAddressResponse) != E_SL_OK)
        {
            user_controlbridge_log("No response to IEEE address request");
            goto done;
        }
        if (u8SequenceNo == psIEEEAddressResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("IEEE Address sequence number received 0x%02X does not match that sent 0x%02X\n", psIEEEAddressResponse->u8SequenceNo, u8SequenceNo);
            if(psIEEEAddressResponse != NULL)
                free(psIEEEAddressResponse);
        }
    }
    //psZCBNode->u64IEEEAddress = be64toh(psIEEEAddressResponse->u64IEEEAddress);
    psZCBNode->u64IEEEAddress = ntoh64(psIEEEAddressResponse->u64IEEEAddress);
    user_controlbridge_log("Short address 0x%04X has IEEE Address 0x%016llX\n", psZCBNode->u16ShortAddress, (unsigned long long int)psZCBNode->u64IEEEAddress);
    eStatus = E_ZCB_OK;

done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    if(psIEEEAddressResponse != NULL)
        free(psIEEEAddressResponse);
    return eStatus;
}




/****************************************************************************
* Function	: eZCB_NodeDescriptorRequest
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_NodeDescriptorRequest(tsZCB_Node *psZCBNode)
{
#pragma pack(1)
    struct _NodeDescriptorRequest
    {
        uint16_t    u16TargetAddress;
    } sNodeDescriptorRequest;
#pragma pack()
#pragma pack(1)
    struct _tNodeDescriptorResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint16_t    u16ManufacturerID;
        uint16_t    u16MaxRxLength;
        uint16_t    u16MaxTxLength;
        uint16_t    u16ServerMask;
        uint8_t     u8DescriptorCapability;
        uint8_t     u8MacCapability;
        uint8_t     u8MaxBufferSize;
        uint16_t    u16Bitfield;
    }*psNodeDescriptorResponse = NULL;
#pragma pack()
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    user_controlbridge_log("Send Node Descriptor request to 0x%04X\n", psZCBNode->u16ShortAddress);

    sNodeDescriptorRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);

    if (eSL_SendMessage(E_SL_MSG_NODE_DESCRIPTOR_REQUEST, sizeof(struct _NodeDescriptorRequest), &sNodeDescriptorRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the node message to arrive */
        if (eSL_MessageWait(E_SL_MSG_NODE_DESCRIPTOR_RESPONSE, 5000, &u16Length, (void**)&psNodeDescriptorResponse) != E_SL_OK)
        {
            user_controlbridge_log("No response to node descriptor request");
            goto done;
        }

        if (u8SequenceNo == psNodeDescriptorResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("Node descriptor sequence number received 0x%02X does not match that sent 0x%02X\n", psNodeDescriptorResponse->u8SequenceNo, u8SequenceNo);
            if(psNodeDescriptorResponse!=NULL)
                free(psNodeDescriptorResponse);
        }
    }

    psZCBNode->u8MacCapability = psNodeDescriptorResponse->u8MacCapability;

    DBG_PrintNode(psZCBNode);
    eStatus = E_ZCB_OK;
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    if(psNodeDescriptorResponse!=NULL)
        free(psNodeDescriptorResponse);
    return eStatus;
}



/****************************************************************************
* Function	: eZCB_SimpleDescriptorRequest
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_SimpleDescriptorRequest(tsZCB_Node *psZCBNode, uint8_t u8Endpoint)
{
#pragma pack(1)
    struct _SimpleDescriptorRequest
    {
        uint16_t    u16TargetAddress;
        uint8_t     u8Endpoint;
    } sSimpleDescriptorRequest;
#pragma pack()
#pragma pack(1)
    struct _tSimpleDescriptorResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint8_t     u8Length;
        uint8_t     u8Endpoint;
        uint16_t    u16ProfileID;
        uint16_t    u16DeviceID;
        uint8_t     u8Bitfields;
        uint8_t     u8InputClusterCount;
        /* Input Clusters */
        /* uint8_t     u8OutputClusterCount;*/
        /* Output Clusters */
    }*psSimpleDescriptorResponse = NULL;
#pragma pack()
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    int iPosition, i;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    user_controlbridge_log("Send Simple Desciptor request for Endpoint %d to 0x%04X", u8Endpoint, psZCBNode->u16ShortAddress);

    sSimpleDescriptorRequest.u16TargetAddress       = htons(psZCBNode->u16ShortAddress);
    sSimpleDescriptorRequest.u8Endpoint             = u8Endpoint;

    if (eSL_SendMessage(E_SL_MSG_SIMPLE_DESCRIPTOR_REQUEST, sizeof(struct _SimpleDescriptorRequest), &sSimpleDescriptorRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE, 500, &u16Length, (void**)&psSimpleDescriptorResponse) != E_SL_OK)
        {
            user_controlbridge_log("No response to simple descriptor request");
            goto done;
        }

        if (u8SequenceNo == psSimpleDescriptorResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("Simple descriptor sequence number received 0x%02X does not match that sent 0x%02X", psSimpleDescriptorResponse->u8SequenceNo, u8SequenceNo);
            //if(psSimpleDescriptorResponse != NULL)
            //    free(psSimpleDescriptorResponse);
        }
    }

    /* Set device ID */
    psZCBNode->u16DeviceID = ntohs(psSimpleDescriptorResponse->u16DeviceID);

    if (eZCB_NodeAddEndpoint(psZCBNode, psSimpleDescriptorResponse->u8Endpoint, ntohs(psSimpleDescriptorResponse->u16ProfileID), NULL) != E_ZCB_OK)
    {
        goto done;
        //eStatus = E_ZCB_ERROR;
    }

    iPosition = sizeof(struct _tSimpleDescriptorResponse);
    for (i = 0; (i < psSimpleDescriptorResponse->u8InputClusterCount) && (iPosition < u16Length); i++)
    {
        uint16_t *psClusterID = (uint16_t *)&((uint8_t*)psSimpleDescriptorResponse)[iPosition];
        if (eZCB_NodeAddCluster(psZCBNode, psSimpleDescriptorResponse->u8Endpoint, ntohs(*psClusterID)) != E_ZCB_OK)
        {
            goto done;
            //eStatus = E_ZCB_ERROR;
        }
        iPosition += sizeof(uint16_t);
    }
    eStatus = E_ZCB_OK;
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    //if(psSimpleDescriptorResponse != NULL)
    //    free(psSimpleDescriptorResponse);
    return eStatus;
}




/****************************************************************************
* Function	: eZCB_ReadAttributeRequest
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_ReadAttributeRequest(tsZCB_Node *psZCBNode, uint16_t u16ClusterID,
                                      uint8_t u8Direction, uint8_t u8ManufacturerSpecific, uint16_t u16ManufacturerID,
                                      uint16_t u16AttributeID, void *pvData)
{
#pragma pack(1)
    struct _ReadAttributeRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Direction;
        uint8_t     u8ManufacturerSpecific;
        uint16_t    u16ManufacturerID;
        uint8_t     u8NumAttributes;
        uint16_t    au16Attribute[1];
    } sReadAttributeRequest;
#pragma pack()
#pragma pack(1)
    struct _ReadAttributeResponseData
    {
        uint8_t     u8Type;
        union
        {
            uint8_t     u8Data;
            uint16_t    u16Data;
            uint32_t    u32Data;
            uint64_t    u64Data;
        } uData;
    } *psReadAttributeResponseData = NULL;
#pragma pack()
#pragma pack(1)
    struct _ReadAttributeResponseAddressed
    {
        uint8_t     u8SequenceNo;
        uint16_t    u16ShortAddress;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint16_t    u16AttributeID;
        uint8_t     u8Status;
        struct _ReadAttributeResponseData sData;
    }*psReadAttributeResponseAddressed = NULL;
#pragma pack()
#pragma pack(1)
    struct _ReadAttributeResponseUnaddressed
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint16_t    u16AttributeID;
        uint8_t     u8Status;
        struct _ReadAttributeResponseData sData;
    }*psReadAttributeResponseUnaddressed = NULL;
#pragma pack()
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    user_controlbridge_log("Send Read Attribute request to 0x%04X\n", psZCBNode->u16ShortAddress);

    if (bZCB_EnableAPSAck)
    {
        sReadAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sReadAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sReadAttributeRequest.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);

    if ((eStatus = eZCB_GetEndpoints(psZCBNode, u16ClusterID, &sReadAttributeRequest.u8SourceEndpoint, &sReadAttributeRequest.u8DestinationEndpoint)) != E_ZCB_OK)
    {
        goto done;
    }

    sReadAttributeRequest.u16ClusterID = htons(u16ClusterID);
    sReadAttributeRequest.u8Direction = u8Direction;
    sReadAttributeRequest.u8ManufacturerSpecific = u8ManufacturerSpecific;
    sReadAttributeRequest.u16ManufacturerID = htons(u16ManufacturerID);
    sReadAttributeRequest.u8NumAttributes = 1;
    sReadAttributeRequest.au16Attribute[0] = htons(u16AttributeID);

    if (eSL_SendMessage(E_SL_MSG_READ_ATTRIBUTE_REQUEST, sizeof(struct _ReadAttributeRequest), &sReadAttributeRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the message to arrive */
        if (eSL_MessageWait(E_SL_MSG_READ_ATTRIBUTE_RESPONSE, 1000, &u16Length, (void**)&psReadAttributeResponseAddressed) != E_SL_OK)
        {
            //if (verbosity > LOG_INFO)
            //{
            //   daemon_log(LOG_DEBUG, "No response to read attribute request");
            //}
            eStatus = E_ZCB_COMMS_FAILED;
            goto done;
        }

        if (u8SequenceNo == psReadAttributeResponseAddressed->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("Read Attribute sequence number received 0x%02X does not match that sent 0x%02X\n", psReadAttributeResponseAddressed->u8SequenceNo, u8SequenceNo);
            if(psReadAttributeResponseAddressed!=NULL)
                free(psReadAttributeResponseAddressed);
        }
    }

    /* Need to cope with older control bridge's which did not embed the short address in the response. */
    psReadAttributeResponseUnaddressed = (struct _ReadAttributeResponseUnaddressed *)psReadAttributeResponseAddressed;

    if ((psReadAttributeResponseAddressed->u16ShortAddress      == htons(psZCBNode->u16ShortAddress)) &&
            (psReadAttributeResponseAddressed->u8Endpoint           == sReadAttributeRequest.u8DestinationEndpoint) &&
            (psReadAttributeResponseAddressed->u16ClusterID         == htons(u16ClusterID)) &&
            (psReadAttributeResponseAddressed->u16AttributeID       == htons(u16AttributeID)))
    {
        user_controlbridge_log("Received addressed read attribute response from 0x%04X\n", psZCBNode->u16ShortAddress);

        if (psReadAttributeResponseAddressed->u8Status != E_ZCB_OK)
        {
            user_controlbridge_log("Read Attribute respose error status: %d\n", psReadAttributeResponseAddressed->u8Status);
            goto done;
        }
        psReadAttributeResponseData = (struct _ReadAttributeResponseData*)&psReadAttributeResponseAddressed->sData;
    }
    else if ((psReadAttributeResponseUnaddressed->u8Endpoint    == sReadAttributeRequest.u8DestinationEndpoint) &&
             (psReadAttributeResponseUnaddressed->u16ClusterID   == htons(u16ClusterID)) &&
             (psReadAttributeResponseUnaddressed->u16AttributeID == htons(u16AttributeID)))
    {
        user_controlbridge_log("Received unaddressed read attribute response from 0x%04X\n", psZCBNode->u16ShortAddress);

        if (psReadAttributeResponseAddressed->u8Status != E_ZCB_OK)
        {
            user_controlbridge_log("Read Attribute respose error status: %d\n", psReadAttributeResponseUnaddressed->u8Status);
            goto done;
        }
        psReadAttributeResponseData = (struct _ReadAttributeResponseData*)&psReadAttributeResponseUnaddressed->sData;
    }
    else
    {
        user_controlbridge_log("No valid read attribute response from 0x%04X\n", psZCBNode->u16ShortAddress);
        goto done;
    }

    /* Copy the data into the pointer passed to us.
    * We assume that the memory pointed to will be the right size for the data that has been requested!
    */
    switch(psReadAttributeResponseData->u8Type)
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        case(E_ZCL_OSTRING):
        case(E_ZCL_CSTRING):
            memcpy(pvData, &psReadAttributeResponseData->uData.u8Data, sizeof(uint8_t));
            eStatus = E_ZCB_OK;
            break;

        case(E_ZCL_LOSTRING):
        case(E_ZCL_LCSTRING):
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
            psReadAttributeResponseData->uData.u16Data = ntohs(psReadAttributeResponseData->uData.u16Data);
            memcpy(pvData, &psReadAttributeResponseData->uData.u16Data, sizeof(uint16_t));
            eStatus = E_ZCB_OK;
            break;

        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
            psReadAttributeResponseData->uData.u32Data = ntohl(psReadAttributeResponseData->uData.u32Data);
            memcpy(pvData, &psReadAttributeResponseData->uData.u32Data, sizeof(uint32_t));
            eStatus = E_ZCB_OK;
            break;

        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
            //psReadAttributeResponseData->uData.u64Data = be64toh(psReadAttributeResponseData->uData.u64Data);
            psReadAttributeResponseData->uData.u64Data = ntoh64(psReadAttributeResponseData->uData.u64Data);
            memcpy(pvData, &psReadAttributeResponseData->uData.u64Data, sizeof(uint64_t));
            eStatus = E_ZCB_OK;
            break;

        default:
            user_controlbridge_log("Unknown attribute data type (%d) received from node 0x%04X", psReadAttributeResponseData->u8Type, psZCBNode->u16ShortAddress);
            break;
    }
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    if(psReadAttributeResponseAddressed!=NULL)
        free(psReadAttributeResponseAddressed);
    return eStatus;
}


teZcbStatus eZCB_WriteAttributeRequest(tsZCB_Node *psZCBNode, uint16_t u16ClusterID,
                                       uint8_t u8Direction, uint8_t u8ManufacturerSpecific, uint16_t u16ManufacturerID,
                                       uint16_t u16AttributeID, teZCL_ZCLAttributeType eType, void *pvData)
{
#pragma pack(1)
    struct _WriteAttributeRequest
    {
        uint8_t     u8TargetAddressMode;//
        uint16_t    u16TargetAddress;//
        uint8_t     u8SourceEndpoint;//
        uint8_t     u8DestinationEndpoint;//
        uint16_t    u16ClusterID;//
        uint8_t     u8Direction;//
        uint8_t     u8ManufacturerSpecific;
        uint16_t    u16ManufacturerID;
        uint8_t     u8NumAttributes;
        uint16_t    u16AttributeID;
        uint8_t     u8Type;
        union
        {
            uint8_t     u8Data;
            uint16_t    u16Data;
            uint32_t    u32Data;
            uint64_t    u64Data;
        } uData;
    } sWriteAttributeRequest;
#pragma pack()
#pragma pack(1)
    struct _WriteAttributeResponse
    {
        /**\todo handle default response properly */
        uint8_t     au8ZCLHeader[3];
        uint16_t    u16MessageType;

        uint8_t     u8SequenceNo;
        uint16_t    u16ShortAddress;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint16_t    u16AttributeID;
        uint8_t     u8Status;
        uint8_t     u8Type;
        union
        {
            uint8_t     u8Data;
            uint16_t    u16Data;
            uint32_t    u32Data;
            uint64_t    u64Data;
        } uData;
    }*psWriteAttributeResponse = NULL;
#pragma pack()

#pragma pack(1)
    struct _DataIndication
    {
        /**\todo handle data indication properly */
        uint8_t     u8ZCBStatus;
        uint16_t    u16ProfileID;
        uint16_t    u16ClusterID;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8SourceAddressMode;
        uint16_t    u16SourceShortAddress; /* OR uint64_t u64IEEEAddress */
        uint8_t     u8DestinationAddressMode;
        uint16_t    u16DestinationShortAddress; /* OR uint64_t u64IEEEAddress */

        uint8_t     u8FrameControl;
        uint8_t     u8SequenceNo;
        uint8_t     u8Command;
        uint8_t     u8Status;
        uint16_t    u16AttributeID;
    }*psDataIndication = NULL;
#pragma pack()

    uint16_t u16Length = sizeof(struct _WriteAttributeRequest) - sizeof(sWriteAttributeRequest.uData);
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    user_controlbridge_log("Send Write Attribute request to 0x%04X\n", psZCBNode->u16ShortAddress);

    if (bZCB_EnableAPSAck)
    {
        sWriteAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sWriteAttributeRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sWriteAttributeRequest.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);

    if ((eStatus = eZCB_GetEndpoints(psZCBNode, u16ClusterID, &sWriteAttributeRequest.u8SourceEndpoint, &sWriteAttributeRequest.u8DestinationEndpoint)) != E_ZCB_OK)
    {
        goto done;
    }

    sWriteAttributeRequest.u16ClusterID             = htons(u16ClusterID);
    sWriteAttributeRequest.u8Direction              = u8Direction;
    sWriteAttributeRequest.u8ManufacturerSpecific   = u8ManufacturerSpecific;
    sWriteAttributeRequest.u16ManufacturerID        = htons(u16ManufacturerID);
    sWriteAttributeRequest.u8NumAttributes          = 1;
    sWriteAttributeRequest.u16AttributeID           = htons(u16AttributeID);
    sWriteAttributeRequest.u8Type                   = (uint8_t)eType;

    switch(eType)
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        case(E_ZCL_OSTRING):
        case(E_ZCL_CSTRING):
            memcpy(&sWriteAttributeRequest.uData.u8Data, pvData, sizeof(uint8_t));
            u16Length += sizeof(uint8_t);
            break;

        case(E_ZCL_LOSTRING):
        case(E_ZCL_LCSTRING):
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
            memcpy(&sWriteAttributeRequest.uData.u16Data, pvData, sizeof(uint16_t));
            sWriteAttributeRequest.uData.u16Data = ntohs(sWriteAttributeRequest.uData.u16Data);
            u16Length += sizeof(uint16_t);
            break;

        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
            memcpy(&sWriteAttributeRequest.uData.u32Data, pvData, sizeof(uint32_t));
            sWriteAttributeRequest.uData.u32Data = ntohl(sWriteAttributeRequest.uData.u32Data);
            u16Length += sizeof(uint32_t);
            break;

        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
            memcpy(&sWriteAttributeRequest.uData.u64Data, pvData, sizeof(uint64_t));
            //sWriteAttributeRequest.uData.u64Data = be64toh(sWriteAttributeRequest.uData.u64Data);
            sWriteAttributeRequest.uData.u64Data = ntoh64(sWriteAttributeRequest.uData.u64Data);
            u16Length += sizeof(uint64_t);
            break;

        default:
            user_controlbridge_log("Unknown attribute data type (%d)", eType);
            return E_ZCB_ERROR;
    }

    if (eSL_SendMessage(E_SL_MSG_WRITE_ATTRIBUTE_REQUEST, u16Length, &sWriteAttributeRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the message to arrive */
        /**\todo handle data indication here for now - BAD Idea! Implement a general case handler in future! */
        if (eSL_MessageWait(E_SL_MSG_DATA_INDICATION, 1000, &u16Length, (void**)&psDataIndication) != E_SL_OK)
        {
            //if (verbosity > LOG_INFO)
            //{
            //    daemon_log(LOG_DEBUG, "No response to write attribute request");
            //}
            eStatus = E_ZCB_COMMS_FAILED;
            goto done;
        }

        user_controlbridge_log("Got data indication\n");

        if (u8SequenceNo == psDataIndication->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("Write Attribute sequence number received 0x%02X does not match that sent 0x%02X\n", psDataIndication->u8SequenceNo, u8SequenceNo);
            if(psDataIndication!=NULL)
                free(psDataIndication);
        }
    }

    user_controlbridge_log("Got write attribute response\n");

    eStatus = psDataIndication->u8Status;

done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);//??????????
    if(psDataIndication!=NULL)
        free(psDataIndication);
    return eStatus;
}



/****************************************************************************
* Function	: eZCB_GetDefaultResponse
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_GetDefaultResponse(uint8_t u8SequenceNo)
{
    uint16_t u16Length;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    tsSL_Msg_DefaultResponse *psDefaultResponse = NULL;

    while (1)
    {
        /* Wait 1 second for a default response message to arrive */
        if (eSL_MessageWait(E_SL_MSG_DEFAULT_RESPONSE, 1000, &u16Length, (void**)&psDefaultResponse) != E_SL_OK)
        {
            user_controlbridge_log("No response to command sequence number %d received", u8SequenceNo);
            goto done;
        }

        if (u8SequenceNo != psDefaultResponse->u8SequenceNo)
        {
            user_controlbridge_log("Default response sequence number received 0x%02X does not match that sent 0x%02X\n", psDefaultResponse->u8SequenceNo, u8SequenceNo);
            if(psDefaultResponse!=NULL)
                free(psDefaultResponse);
        }
        else
        {
            user_controlbridge_log("Default response for message sequence number 0x%02X status is %d\n", psDefaultResponse->u8SequenceNo, psDefaultResponse->u8Status);
            eStatus = psDefaultResponse->u8Status;
            break;
        }
    }
done:
    if(psDefaultResponse!=NULL)
        free(psDefaultResponse);
    return eStatus;
}




/****************************************************************************
* Function	: eZCB_AddGroupMembership
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_AddGroupMembership(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress)
{
#pragma pack(1)
    struct _AddGroupMembershipRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16GroupAddress;
    } sAddGroupMembershipRequest;
#pragma pack()
#pragma pack(1)
    struct _sAddGroupMembershipResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Status;
        uint16_t    u16GroupAddress;
    }*psAddGroupMembershipResponse = NULL;
#pragma pack()
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    user_controlbridge_log("Send add group membership 0x%04X request to 0x%04X\n", u16GroupAddress, psZCBNode->u16ShortAddress);

    if (bZCB_EnableAPSAck)
    {
        sAddGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sAddGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sAddGroupMembershipRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);

    if ((eStatus = eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_GROUPS, &sAddGroupMembershipRequest.u8SourceEndpoint, &sAddGroupMembershipRequest.u8DestinationEndpoint)) != E_ZCB_OK)
    {
        return eStatus;
    }

    sAddGroupMembershipRequest.u16GroupAddress = htons(u16GroupAddress);

    if (eSL_SendMessage(E_SL_MSG_ADD_GROUP_REQUEST, sizeof(struct _AddGroupMembershipRequest), &sAddGroupMembershipRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the add group response message to arrive */
        if (eSL_MessageWait(E_SL_MSG_ADD_GROUP_RESPONSE, 1000, &u16Length, (void**)&psAddGroupMembershipResponse) != E_SL_OK)
        {
            user_controlbridge_log("No response to add group membership request");
            goto done;
        }

        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psAddGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("Add group membership sequence number received 0x%02X does not match that sent 0x%02X\n", psAddGroupMembershipResponse->u8SequenceNo, u8SequenceNo);
            if(psAddGroupMembershipResponse!=NULL)
                free(psAddGroupMembershipResponse);
        }
    }

    user_controlbridge_log("Add group membership 0x%04X on Node 0x%04X status: %d\n", u16GroupAddress, psZCBNode->u16ShortAddress, psAddGroupMembershipResponse->u8Status);

    eStatus = psAddGroupMembershipResponse->u8Status;

done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    if(psAddGroupMembershipResponse!=NULL)
        free(psAddGroupMembershipResponse);
    return eStatus;
}



/****************************************************************************
* Function	: eZCB_RemoveGroupMembership
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_RemoveGroupMembership(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress)
{
#pragma pack(1)
    struct _RemoveGroupMembershipRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16GroupAddress;
    } sRemoveGroupMembershipRequest;
#pragma pack()
#pragma pack(1)
    struct _sRemoveGroupMembershipResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Status;
        uint16_t    u16GroupAddress;
    }*psRemoveGroupMembershipResponse = NULL;
#pragma pack()
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    user_controlbridge_log("Send remove group membership 0x%04X request to 0x%04X\n", u16GroupAddress, psZCBNode->u16ShortAddress);

    if (bZCB_EnableAPSAck)
    {
        sRemoveGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sRemoveGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sRemoveGroupMembershipRequest.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);

    if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_GROUPS, &sRemoveGroupMembershipRequest.u8SourceEndpoint, &sRemoveGroupMembershipRequest.u8DestinationEndpoint) != E_ZCB_OK)
    {
        return E_ZCB_ERROR;
    }

    sRemoveGroupMembershipRequest.u16GroupAddress = htons(u16GroupAddress);

    if (eSL_SendMessage(E_SL_MSG_REMOVE_GROUP_REQUEST, sizeof(struct _RemoveGroupMembershipRequest), &sRemoveGroupMembershipRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the remove group response message to arrive */
        if (eSL_MessageWait(E_SL_MSG_REMOVE_GROUP_RESPONSE, 1000, &u16Length, (void**)&psRemoveGroupMembershipResponse) != E_SL_OK)
        {
            user_controlbridge_log("No response to remove group membership request");
            goto done;
        }

        /* Work around bug in Zigbee */
        if (u8SequenceNo != psRemoveGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("Remove group membership sequence number received 0x%02X does not match that sent 0x%02X\n", psRemoveGroupMembershipResponse->u8SequenceNo, u8SequenceNo);
            if(psRemoveGroupMembershipResponse!=NULL)
                free(psRemoveGroupMembershipResponse);
        }
    }

    user_controlbridge_log("Remove group membership 0x%04X on Node 0x%04X status: %d\n", u16GroupAddress, psZCBNode->u16ShortAddress, psRemoveGroupMembershipResponse->u8Status);

    eStatus = psRemoveGroupMembershipResponse->u8Status;

done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    if(psRemoveGroupMembershipResponse!=NULL)
        free(psRemoveGroupMembershipResponse);
    return eStatus;
}




/****************************************************************************
* Function	: eZCB_GetGroupMembership
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_GetGroupMembership(tsZCB_Node *psZCBNode)
{
#pragma pack(1)
    struct _GetGroupMembershipRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8GroupCount;
        uint16_t    au16GroupList[0];
    } sGetGroupMembershipRequest;
#pragma pack()
#pragma pack(1)
    struct _sGetGroupMembershipResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Capacity;
        uint8_t     u8GroupCount;
        uint16_t    au16GroupList[255];
    }*psGetGroupMembershipResponse = NULL;
#pragma pack()
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;
    int i;

    user_controlbridge_log("Send get group membership request to 0x%04X\n", psZCBNode->u16ShortAddress);

    if (bZCB_EnableAPSAck)
    {
        sGetGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sGetGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sGetGroupMembershipRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);

    if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_GROUPS, &sGetGroupMembershipRequest.u8SourceEndpoint, &sGetGroupMembershipRequest.u8DestinationEndpoint) != E_ZCB_OK)
    {
        return E_ZCB_ERROR;
    }

    sGetGroupMembershipRequest.u8GroupCount     = 0;

    if (eSL_SendMessage(E_SL_MSG_GET_GROUP_MEMBERSHIP_REQUEST, sizeof(struct _GetGroupMembershipRequest), &sGetGroupMembershipRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_GET_GROUP_MEMBERSHIP_RESPONSE, 1000, &u16Length, (void**)&psGetGroupMembershipResponse) != E_SL_OK)
        {
            user_controlbridge_log("No response to group membership request");
            goto done;
        }

        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psGetGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("Get group membership sequence number received 0x%02X does not match that sent 0x%02X\n", psGetGroupMembershipResponse->u8SequenceNo, u8SequenceNo);
            if(psGetGroupMembershipResponse!=NULL)
                free(psGetGroupMembershipResponse);
        }
    }

    user_controlbridge_log("Node 0x%04X is in %d/%d groups\n", psZCBNode->u16ShortAddress,
                           psGetGroupMembershipResponse->u8GroupCount,
                           psGetGroupMembershipResponse->u8GroupCount + psGetGroupMembershipResponse->u8Capacity);

    if (eZCB_NodeClearGroups(psZCBNode) != E_ZCB_OK)
    {
        goto done;
    }

    for(i = 0; i < psGetGroupMembershipResponse->u8GroupCount; i++)
    {
        psGetGroupMembershipResponse->au16GroupList[i] = ntohs(psGetGroupMembershipResponse->au16GroupList[i]);
        user_controlbridge_log("  Group ID 0x%04X\n", psGetGroupMembershipResponse->au16GroupList[i]);
        if ((eStatus = eZCB_NodeAddGroup(psZCBNode, psGetGroupMembershipResponse->au16GroupList[i])) != E_ZCB_OK)
        {
            goto done;
        }
    }
    eStatus = E_ZCB_OK;
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    if(psGetGroupMembershipResponse!=NULL)
        free(psGetGroupMembershipResponse);
    return eStatus;
}


/****************************************************************************
* Function	: eZCB_ClearGroupMembership
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_ClearGroupMembership(tsZCB_Node *psZCBNode)
{
#pragma pack(1)
    struct _ClearGroupMembershipRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
    } sClearGroupMembershipRequest;
#pragma pack()
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    user_controlbridge_log("Send clear group membership request to 0x%04X\n", psZCBNode->u16ShortAddress);

    if (bZCB_EnableAPSAck)
    {
        sClearGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sClearGroupMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sClearGroupMembershipRequest.u16TargetAddress      = htons(psZCBNode->u16ShortAddress);

    if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_GROUPS, &sClearGroupMembershipRequest.u8SourceEndpoint, &sClearGroupMembershipRequest.u8DestinationEndpoint) != E_ZCB_OK)
    {
        return E_ZCB_ERROR;
    }

    if (eSL_SendMessage(E_SL_MSG_REMOVE_ALL_GROUPS, sizeof(struct _ClearGroupMembershipRequest), &sClearGroupMembershipRequest, NULL) != E_SL_OK)
    {
        goto done;
    }
    eStatus = E_ZCB_OK;

done:
    return eStatus;
}



/****************************************************************************
* Function	: eZCB_RemoveScene
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_RemoveScene(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8SceneID)
{
#pragma pack(1)
    struct _RemoveSceneRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16GroupAddress;
        uint8_t     u8SceneID;
    } sRemoveSceneRequest;
#pragma pack()
#pragma pack(1)
    struct _sStoreSceneResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Status;
        uint16_t    u16GroupAddress;
        uint8_t     u8SceneID;
    } *psRemoveSceneResponse = NULL;
#pragma pack()
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    user_controlbridge_log("Send remove scene %d (Group 0x%04X) for Endpoint %d to 0x%04X\n",
                           u8SceneID, u16GroupAddress, sRemoveSceneRequest.u8DestinationEndpoint, psZCBNode->u16ShortAddress);

    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sRemoveSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sRemoveSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sRemoveSceneRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);

        if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_SCENES, &sRemoveSceneRequest.u8SourceEndpoint, &sRemoveSceneRequest.u8DestinationEndpoint) != E_ZCB_OK)
        {
            return E_ZCB_ERROR;
        }
    }
    else
    {
        sRemoveSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sRemoveSceneRequest.u16TargetAddress      = htons(u16GroupAddress);
        sRemoveSceneRequest.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;

        if (eZCB_GetEndpoints(NULL, E_ZB_CLUSTERID_SCENES, &sRemoveSceneRequest.u8SourceEndpoint, NULL) != E_ZCB_OK)
        {
            return E_ZCB_ERROR;
        }
    }

    sRemoveSceneRequest.u16GroupAddress  = htons(u16GroupAddress);
    sRemoveSceneRequest.u8SceneID        = u8SceneID;

    if (eSL_SendMessage(E_SL_MSG_REMOVE_SCENE, sizeof(struct _RemoveSceneRequest), &sRemoveSceneRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_REMOVE_SCENE_RESPONSE, 1000, &u16Length, (void**)&psRemoveSceneResponse) != E_SL_OK)
        {
            user_controlbridge_log("No response to remove scene request");
            goto done;
        }

        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psGetGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("Remove scene sequence number received 0x%02X does not match that sent 0x%02X\n", psRemoveSceneResponse->u8SequenceNo, u8SequenceNo);
            if(psRemoveSceneResponse!=NULL)
                free(psRemoveSceneResponse);
        }
    }

    user_controlbridge_log("Remove scene %d (Group0x%04X) on Node 0x%04X status: %d\n",
                           psRemoveSceneResponse->u8SceneID, ntohs(psRemoveSceneResponse->u16GroupAddress), psZCBNode->u16ShortAddress, psRemoveSceneResponse->u8Status);

    eStatus = psRemoveSceneResponse->u8Status;
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    if(psRemoveSceneResponse!=NULL)
        free(psRemoveSceneResponse);
    return eStatus;
}



/****************************************************************************
* Function	: eZCB_StoreScene
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_StoreScene(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8SceneID)
{
#pragma pack(1)
    struct _StoreSceneRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16GroupAddress;
        uint8_t     u8SceneID;
    } sStoreSceneRequest;
#pragma pack()
#pragma pack(1)
    struct _sStoreSceneResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Status;
        uint16_t    u16GroupAddress;
        uint8_t     u8SceneID;
    }*psStoreSceneResponse = NULL;
#pragma pack()
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    user_controlbridge_log("Send store scene %d (Group 0x%04X)\n",
                           u8SceneID, u16GroupAddress);

    if (psZCBNode)
    {
        if (bZCB_EnableAPSAck)
        {
            sStoreSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sStoreSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sStoreSceneRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);

        if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_SCENES, &sStoreSceneRequest.u8SourceEndpoint, &sStoreSceneRequest.u8DestinationEndpoint) != E_ZCB_OK)
        {
            return E_ZCB_ERROR;
        }
    }
    else
    {
        sStoreSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        sStoreSceneRequest.u16TargetAddress      = htons(u16GroupAddress);
        sStoreSceneRequest.u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;

        if (eZCB_GetEndpoints(NULL, E_ZB_CLUSTERID_SCENES, &sStoreSceneRequest.u8SourceEndpoint, NULL) != E_ZCB_OK)
        {
            return E_ZCB_ERROR;
        }
    }

    sStoreSceneRequest.u16GroupAddress  = htons(u16GroupAddress);
    sStoreSceneRequest.u8SceneID        = u8SceneID;

    if (eSL_SendMessage(E_SL_MSG_STORE_SCENE, sizeof(struct _StoreSceneRequest), &sStoreSceneRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the descriptor message to arrive */
        if (eSL_MessageWait(E_SL_MSG_STORE_SCENE_RESPONSE, 1000, &u16Length, (void**)&psStoreSceneResponse) != E_SL_OK)
        {
            user_controlbridge_log("No response to store scene request");
            goto done;
        }

        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psGetGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("Store scene sequence number received 0x%02X does not match that sent 0x%02X\n", psStoreSceneResponse->u8SequenceNo, u8SequenceNo);
            if(psStoreSceneResponse!=NULL)
                free(psStoreSceneResponse);
        }
    }

    user_controlbridge_log("Store scene %d (Group0x%04X) on Node 0x%04X status: %d\n",
                           psStoreSceneResponse->u8SceneID, ntohs(psStoreSceneResponse->u16GroupAddress), ntohs(psZCBNode->u16ShortAddress), psStoreSceneResponse->u8Status);

    eStatus = psStoreSceneResponse->u8Status;
done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    if(psStoreSceneResponse!=NULL)
        free(psStoreSceneResponse);
    return eStatus;
}



/****************************************************************************
* Function	: eZCB_RecallScene
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_RecallScene(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t u8SceneID)
{
    uint8_t         u8SequenceNo;
#pragma pack(1)
    struct _RecallSceneRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16GroupAddress;
        uint8_t     u8SceneID;
    } sRecallSceneRequest;
#pragma pack()
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    if (psZCBNode)
    {
        user_controlbridge_log("Send recall scene %d (Group 0x%04X) to 0x%04X\n",
                               u8SceneID, u16GroupAddress, psZCBNode->u16ShortAddress);

        if (bZCB_EnableAPSAck)
        {
            sRecallSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        }
        else
        {
            sRecallSceneRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
        }
        sRecallSceneRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);

        if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_SCENES, &sRecallSceneRequest.u8SourceEndpoint, &sRecallSceneRequest.u8DestinationEndpoint) != E_ZCB_OK)
        {
            return E_ZCB_ERROR;
        }
    }
    else
    {
        sRecallSceneRequest.u8TargetAddressMode  = E_ZB_ADDRESS_MODE_GROUP;
        sRecallSceneRequest.u16TargetAddress     = htons(u16GroupAddress);
        sRecallSceneRequest.u8DestinationEndpoint= ZB_DEFAULT_ENDPOINT_ZLL;

        user_controlbridge_log("Send recall scene %d (Group 0x%04X) for Endpoint %d to 0x%04X\n",
                               u8SceneID, u16GroupAddress, sRecallSceneRequest.u8DestinationEndpoint, u16GroupAddress);

        if (eZCB_GetEndpoints(NULL, E_ZB_CLUSTERID_SCENES, &sRecallSceneRequest.u8SourceEndpoint, NULL) != E_ZCB_OK)
        {
            return E_ZCB_ERROR;
        }
    }

    sRecallSceneRequest.u16GroupAddress  = htons(u16GroupAddress);
    sRecallSceneRequest.u8SceneID        = u8SceneID;

    if (eSL_SendMessage(E_SL_MSG_RECALL_SCENE, sizeof(struct _RecallSceneRequest), &sRecallSceneRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    if (psZCBNode)
    {
        eStatus = eZCB_GetDefaultResponse(u8SequenceNo);
    }
    else
    {
        eStatus = E_ZCB_OK;
    }
done:
    return eStatus;
}



/****************************************************************************
* Function	: eZCB_GetSceneMembership
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_GetSceneMembership(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress, uint8_t *pu8NumScenes, uint8_t **pau8Scenes)
{
#pragma pack(1)
    struct _GetSceneMembershipRequest
    {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16GroupAddress;
    } sGetSceneMembershipRequest;
#pragma pack()
#pragma pack(1)
    struct _sGetSceneMembershipResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint8_t     u8Status;
        uint8_t     u8Capacity;
        uint16_t    u16GroupAddress;
        uint8_t     u8NumScenes;
        uint8_t     au8Scenes[255];
    }*psGetSceneMembershipResponse = NULL;
#pragma pack()
    uint16_t u16Length;
    uint8_t u8SequenceNo;
    teZcbStatus eStatus = E_ZCB_COMMS_FAILED;

    user_controlbridge_log("Send get scene membership for group 0x%04X to 0x%04X\n",
                           u16GroupAddress, psZCBNode->u16ShortAddress);

    if (bZCB_EnableAPSAck)
    {
        sGetSceneMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
    }
    else
    {
        sGetSceneMembershipRequest.u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT_NO_ACK;
    }
    sGetSceneMembershipRequest.u16TargetAddress     = htons(psZCBNode->u16ShortAddress);

    if (eZCB_GetEndpoints(psZCBNode, E_ZB_CLUSTERID_SCENES, &sGetSceneMembershipRequest.u8SourceEndpoint, &sGetSceneMembershipRequest.u8DestinationEndpoint) != E_ZCB_OK)
    {
        return E_ZCB_ERROR;
    }

    sGetSceneMembershipRequest.u16GroupAddress  = htons(u16GroupAddress);

    if (eSL_SendMessage(E_SL_MSG_SCENE_MEMBERSHIP_REQUEST, sizeof(struct _GetSceneMembershipRequest), &sGetSceneMembershipRequest, &u8SequenceNo) != E_SL_OK)
    {
        goto done;
    }

    while (1)
    {
        /* Wait 1 second for the response to arrive */
        if (eSL_MessageWait(E_SL_MSG_SCENE_MEMBERSHIP_RESPONSE, 1000, &u16Length, (void**)&psGetSceneMembershipResponse) != E_SL_OK)
        {
            user_controlbridge_log("No response to get scene membership request");
            goto done;
        }

        /* Work around bug in Zigbee */
        if (1)//u8SequenceNo != psGetGroupMembershipResponse->u8SequenceNo)
        {
            break;
        }
        else
        {
            user_controlbridge_log("Get scene membership sequence number received 0x%02X does not match that sent 0x%02X\n", psGetSceneMembershipResponse->u8SequenceNo, u8SequenceNo);
            if(psGetSceneMembershipResponse!=NULL)
                free(psGetSceneMembershipResponse);
        }
    }

    user_controlbridge_log("Scene membership for group 0x%04X on Node 0x%04X status: %d\n",
                           ntohs(psGetSceneMembershipResponse->u16GroupAddress), psZCBNode->u16ShortAddress, psGetSceneMembershipResponse->u8Status);

    eStatus = psGetSceneMembershipResponse->u8Status;

    if (eStatus == E_ZCB_OK)
    {
        int i;
        user_controlbridge_log("Node 0x%04X, group 0x%04X is in %d scenes\n",
                               psZCBNode->u16ShortAddress, ntohs(psGetSceneMembershipResponse->u16GroupAddress), psGetSceneMembershipResponse->u8NumScenes);

        *pu8NumScenes = psGetSceneMembershipResponse->u8NumScenes;
        *pau8Scenes = realloc(*pau8Scenes, *pu8NumScenes * sizeof(uint8_t));
        if (!pu8NumScenes)
        {
            return E_ZCB_ERROR_NO_MEM;
        }

        for (i = 0; i < psGetSceneMembershipResponse->u8NumScenes; i++)
        {
            user_controlbridge_log("  Scene 0x%02X\n", psGetSceneMembershipResponse->au8Scenes[i]);
            (*pau8Scenes)[i] = psGetSceneMembershipResponse->au8Scenes[i];
        }
    }

done:
    vZCB_NodeUpdateComms(psZCBNode, eStatus);
    if(psGetSceneMembershipResponse!=NULL)
        free(psGetSceneMembershipResponse);
    return eStatus;
}




/****************************************************************************
* Function	: eZCB_GetEndpoints
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_GetEndpoints(tsZCB_Node *psZCBNode, eZigbee_ClusterID eClusterID, uint8_t *pu8Src, uint8_t *pu8Dst)
{
    tsZCB_Node          *psControlBridge;
    tsZCB_NodeEndpoint  *psSourceEndpoint;
    tsZCB_NodeEndpoint  *psDestinationEndpoint;

    if (pu8Src)
    {
        psControlBridge = psZCB_FindNodeControlBridge();
        if (!psControlBridge)
        {
            return E_ZCB_ERROR;
        }
        psSourceEndpoint = psZCB_NodeFindEndpoint(psControlBridge, eClusterID);
        if (!psSourceEndpoint)
        {
            user_controlbridge_log("Cluster ID 0x%04X not found on control bridge\n", eClusterID);
            mico_rtos_lock_mutex(&psControlBridge->sLock);
            return E_ZCB_UNKNOWN_CLUSTER;
        }

        *pu8Src = psSourceEndpoint->u8Endpoint;
        mico_rtos_lock_mutex(&psControlBridge->sLock);
    }

    if (pu8Dst)
    {
        if (!psZCBNode)
        {
            return E_ZCB_ERROR;
        }
        psDestinationEndpoint = psZCB_NodeFindEndpoint(psZCBNode, eClusterID);

        if (!psDestinationEndpoint)
        {
            user_controlbridge_log("Cluster ID 0x%04X not found on node 0x%04X\n", eClusterID, psZCBNode->u16ShortAddress);
            return E_ZCB_UNKNOWN_CLUSTER;
        }
        *pu8Dst = psDestinationEndpoint->u8Endpoint;
    }
    return E_ZCB_OK;
}



/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
* Function	: ZCB_HandleNodeClusterList
* Description	: Handle Node Cluster List
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void ZCB_HandleNodeClusterList(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    int iPosition;
    int iCluster = 0;

#pragma pack(1)
    struct _tsClusterList
    {
        uint8_t     u8Endpoint;
        uint16_t    u16ProfileID;
        uint16_t    au16ClusterList[255];
    }*psClusterList = (struct _tsClusterList *)pvMessage;		//cluster 列表
#pragma pack()
    psClusterList->u16ProfileID = ntohs(psClusterList->u16ProfileID);

//    user_controlbridge_log("Cluster list for endpoint %d, profile ID 0x%4X",
//                           psClusterList->u8Endpoint,
//                           psClusterList->u16ProfileID);

    mico_rtos_lock_mutex(&sZCB_Network.sNodes.sLock);

    //增加 endpoint
    if (eZCB_NodeAddEndpoint(&sZCB_Network.sNodes, psClusterList->u8Endpoint, psClusterList->u16ProfileID, NULL) != E_ZCB_OK)
    {
        goto done;
    }

    iPosition = sizeof(uint8_t) + sizeof(uint16_t);
    while(iPosition < u16Length)
    {
        //增加 cluster
        if (eZCB_NodeAddCluster(&sZCB_Network.sNodes, psClusterList->u8Endpoint, ntohs(psClusterList->au16ClusterList[iCluster])) != E_ZCB_OK)
        {
            goto done;
        }
        iPosition += sizeof(uint16_t);
        iCluster++;
    }

    //DBG_PrintNode(&sZCB_Network.sNodes);
done:
    mico_rtos_unlock_mutex(&sZCB_Network.sNodes.sLock);
}



/****************************************************************************
* Function	: ZCB_HandleNodeClusterAttributeList
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void ZCB_HandleNodeClusterAttributeList(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    int iPosition;
    int iAttribute = 0;
#pragma pack(1)
    struct _tsClusterAttributeList
    {
        uint8_t     u8Endpoint;
        uint16_t    u16ProfileID;
        uint16_t    u16ClusterID;
        uint16_t    au16AttributeList[255];
    }*psClusterAttributeList = (struct _tsClusterAttributeList *)pvMessage;
#pragma pack()
    psClusterAttributeList->u16ProfileID = ntohs(psClusterAttributeList->u16ProfileID);
    psClusterAttributeList->u16ClusterID = ntohs(psClusterAttributeList->u16ClusterID);

//    user_controlbridge_log("Cluster attribute list for endpoint %d, cluster 0x%04X, profile ID 0x%4X\n",
//                           psClusterAttributeList->u8Endpoint,
//                           psClusterAttributeList->u16ClusterID,
//                           psClusterAttributeList->u16ProfileID);

    mico_rtos_lock_mutex(&sZCB_Network.sNodes.sLock);

    iPosition = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t);
    while(iPosition < u16Length)
    {
        if (eZCB_NodeAddAttribute(&sZCB_Network.sNodes, psClusterAttributeList->u8Endpoint,
                                  psClusterAttributeList->u16ClusterID, ntohs(psClusterAttributeList->au16AttributeList[iAttribute])) != E_ZCB_OK)
        {
            goto done;
        }
        iPosition += sizeof(uint16_t);
        iAttribute++;
    }

    //DBG_PrintNode(&sZCB_Network.sNodes);

done:
    mico_rtos_unlock_mutex(&sZCB_Network.sNodes.sLock);
}



/****************************************************************************
* Function	: ZCB_HandleNodeCommandIDList
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void ZCB_HandleNodeCommandIDList(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    int iPosition;
    int iCommand = 0;
#pragma pack(1)
    struct _tsCommandIDList
    {
        uint8_t     u8Endpoint;
        uint16_t    u16ProfileID;
        uint16_t    u16ClusterID;
        uint8_t     au8CommandList[255];
    }*psCommandIDList = (struct _tsCommandIDList *)pvMessage;
#pragma pack()
    psCommandIDList->u16ProfileID = ntohs(psCommandIDList->u16ProfileID);
    psCommandIDList->u16ClusterID = ntohs(psCommandIDList->u16ClusterID);

//    user_controlbridge_log("Command ID list for endpoint %d, cluster 0x%04X, profile ID 0x%4X\n",
//                           psCommandIDList->u8Endpoint,
//                           psCommandIDList->u16ClusterID,
//                           psCommandIDList->u16ProfileID);

    mico_rtos_lock_mutex(&sZCB_Network.sNodes.sLock);

    iPosition = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t);
    while(iPosition < u16Length)
    {
        if (eZCB_NodeAddCommand(&sZCB_Network.sNodes, psCommandIDList->u8Endpoint,
                                psCommandIDList->u16ClusterID, psCommandIDList->au8CommandList[iCommand]) != E_ZCB_OK)
        {
            goto done;
        }
        iPosition += sizeof(uint8_t);
        iCommand++;
    }

    //DBG_PrintNode(&sZCB_Network.sNodes);
done:
    mico_rtos_unlock_mutex(&sZCB_Network.sNodes.sLock);
}


/****************************************************************************
* Function	: ZCB_HandleRestartProvisioned
* Description	: 处理 Restart Provisioned
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void ZCB_HandleRestartProvisioned(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    const char *pcStatus = NULL;

#pragma pack(1)
    struct _tsWarmRestart
    {
        uint8_t     u8Status;
    }*psWarmRestart = (struct _tsWarmRestart *)pvMessage;
#pragma pack()
    switch (psWarmRestart->u8Status)
    {
#define STATUS(a, b) case(a): pcStatus = b; break
            STATUS(0, "STARTUP");
            STATUS(1, "WAIT_START");
            STATUS(2, "NFN_START");
            STATUS(3, "DISCOVERY");
            STATUS(4, "NETWORK_INIT");
            STATUS(5, "RESCAN");
            STATUS(6, "RUNNING");
#undef STATUS
        default:
            pcStatus = "Unknown";
    }
    user_controlbridge_log("Control bridge restarted, status %d (%s)", psWarmRestart->u8Status, pcStatus);
    return;
}



/****************************************************************************
* Function	: ZCB_HandleRestartFactoryNew
* Description	: 处理出厂复位
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void ZCB_HandleRestartFactoryNew(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    const char *pcStatus = NULL;

#pragma pack(1)
    struct _tsWarmRestart
    {
        uint8_t     u8Status;
    }*psWarmRestart = (struct _tsWarmRestart *)pvMessage;
#pragma pack()
    switch (psWarmRestart->u8Status)
    {
#define STATUS(a, b) case(a): pcStatus = b; break
            STATUS(0, "STARTUP");
            STATUS(1, "WAIT_START");
            STATUS(2, "NFN_START");
            STATUS(3, "DISCOVERY");
            STATUS(4, "NETWORK_INIT");
            STATUS(5, "RESCAN");
            STATUS(6, "RUNNING");
#undef STATUS
        default:
            pcStatus = "Unknown";
    }
    user_controlbridge_log("Control bridge factory new restart, status %d (%s)", psWarmRestart->u8Status, pcStatus);

    eZCB_ConfigureControlBridge();
    return;
}


/****************************************************************************
* Function	: ZCB_HandleNetworkJoined
* Description	: 处理网络加入
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void ZCB_HandleNetworkJoined(void *pvUser, uint16_t u16Length, void *pvMessage)
{
#pragma pack(1)
    struct _tsNetworkJoinedFormedShort
    {
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint64_t    u64IEEEAddress;
        uint8_t     u8Channel;
    } *psMessageShort = (struct _tsNetworkJoinedFormedShort *)pvMessage;
#pragma pack()
#pragma pack(1)
    struct _tsNetworkJoinedFormedExtended
    {
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint64_t    u64IEEEAddress;
        uint8_t     u8Channel;
        uint64_t    u64PanID;
        uint16_t    u16PanID;
    }*psMessageExt = (struct _tsNetworkJoinedFormedExtended *)pvMessage;
#pragma pack()
    tsZcbEvent *psEvent;

    psMessageShort->u16ShortAddress = ntohs(psMessageShort->u16ShortAddress);
    //psMessageShort->u64IEEEAddress  = be64toh(psMessageShort->u64IEEEAddress);
    psMessageShort->u64IEEEAddress  = ntoh64(psMessageShort->u64IEEEAddress);

    if (u16Length == sizeof(struct _tsNetworkJoinedFormedExtended))
    {
        //psMessageExt->u64PanID      = be64toh(psMessageExt->u64PanID);
        psMessageExt->u64PanID      = ntoh64(psMessageExt->u64PanID);
        psMessageExt->u16PanID      = ntohs(psMessageExt->u16PanID);

        user_controlbridge_log("Network %s on channel %d. Control bridge address 0x%04X (0x%016llX). PAN ID 0x%04X (0x%016llX)",
                               psMessageExt->u8Status == 0 ? "joined" : "formed",
                               psMessageExt->u8Channel,
                               psMessageExt->u16ShortAddress,
                               (unsigned long long int)psMessageExt->u64IEEEAddress,
                               psMessageExt->u16PanID,
                               (unsigned long long int)psMessageExt->u64PanID);

        /* Update global network information */
        eChannelInUse = psMessageExt->u8Channel;
        u64PanIDInUse = psMessageExt->u64PanID;
        u16PanIDInUse = psMessageExt->u16PanID;
    }
    else
    {
        user_controlbridge_log("Network %s on channel %d. Control bridge address 0x%04X (0x%016llX)",
                               psMessageShort->u8Status == 0 ? "joined" : "formed",
                               psMessageShort->u8Channel,
                               psMessageShort->u16ShortAddress,
                               (unsigned long long int)psMessageShort->u64IEEEAddress);
    }


    /* Control bridge joined the network - initialise its data in the network structure */
    mico_rtos_lock_mutex(&sZCB_Network.sNodes.sLock);

    sZCB_Network.sNodes.u16DeviceID     = E_ZB_DEVICEID_CONTROLBRIDGE;
    sZCB_Network.sNodes.u16ShortAddress = psMessageShort->u16ShortAddress;
    sZCB_Network.sNodes.u64IEEEAddress  = psMessageShort->u64IEEEAddress;
    sZCB_Network.sNodes.u8MacCapability = E_ZB_MAC_CAPABILITY_RXON_WHEN_IDLE;

    user_controlbridge_log("Node Joined 0x%04X (0x%016llX)",
                           sZCB_Network.sNodes.u16ShortAddress,
                           (unsigned long long int)sZCB_Network.sNodes.u64IEEEAddress);


    psEvent = malloc(sizeof(tsZcbEvent));
    if (!psEvent)
    {
        user_controlbridge_log("Memory allocation failure allocating event");
        return;
    }

    psEvent->eEvent = (psMessageShort->u8Status == 0 ? E_ZCB_EVENT_NETWORK_JOINED : E_ZCB_EVENT_NETWORK_FORMED);

    mico_rtos_unlock_mutex(&sZCB_Network.sNodes.sLock);

    teZcbStatus status ;
    status = eZCB_HandleZcbEvent(psEvent);
    if(status != E_ZCB_OK)
        user_controlbridge_log("Handle Zcb Event err");


    //if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != TRUE)
    //{
    //   user_controlbridge_log("Error queue'ing event\n");
    //    free(psEvent);
    //}

    /* Test thermostat */
#if 0
    {
        tsZCB_Node *psZCBNode;
        if (eZCB_AddNode(0x1234, 0x1234ll, 0, 0, &psZCBNode) == E_ZCB_OK)
        {
            psEvent = malloc(sizeof(tsZcbEvent));

            if (!psEvent)
            {
                user_controlbridge_log("Memory allocation failure allocating event");
                return;
            }
            psEvent->eEvent                                 = E_ZCB_EVENT_DEVICE_MATCH;
            psEvent->uData.sDeviceMatch.u16ShortAddress     = 0x1234;

            psZCBNode->u16DeviceID = 0x0301;

            mico_rtos_unlock_mutex(&psZCBNode->sLock);


            if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != E_UTILS_OK)
            {
                user_controlbridge_log("Error queue'ing event\n");
                free(psEvent);
            }
            else
            {
                user_controlbridge_log("Test device added\n");
            }
        }
    }
#endif
    return;
}


/****************************************************************************
* Function	: ZCB_HandleDeviceAnnounce
* Description	: 处理设备 Announce
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void ZCB_HandleDeviceAnnounce(void *pvUser, uint16_t u16Length, void *pvMessage)
{

    tsZCB_Node *psZCBNode;
    tsZcbEvent *psEvent = NULL;
    teZcbStatus status = E_ZCB_ERROR;
#pragma pack(1)
    struct _tsDeviceAnnounce
    {
        uint16_t    u16ShortAddress;
        uint64_t    u64IEEEAddress;
        uint8_t     u8MacCapability;
    }*psMessage = (struct _tsDeviceAnnounce *)pvMessage;
#pragma pack()
    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u64IEEEAddress   = ntoh64(psMessage->u64IEEEAddress);

    user_controlbridge_log("Device Joined, Address 0x%04X (0x%016llX). Mac Capability Mask 0x%02X",
                           psMessage->u16ShortAddress,
                           (unsigned long long int)psMessage->u64IEEEAddress,
                           psMessage->u8MacCapability
                          );

    psEvent = malloc(sizeof(tsZcbEvent));
    if (!psEvent)
    {
        user_controlbridge_log("Memory allocation failure allocating event");
        return;
    }

    if (eZCB_AddNode(psMessage->u16ShortAddress, psMessage->u64IEEEAddress, 0, psMessage->u8MacCapability, &psZCBNode) == E_ZCB_OK)
    {
        psEvent->eEvent                                 = E_ZCB_EVENT_DEVICE_ANNOUNCE;
        psEvent->uData.sDeviceAnnounce.u16ShortAddress  = psZCBNode->u16ShortAddress;

        psZCBNode->u8MacCapability = psMessage->u8MacCapability;

        mico_rtos_unlock_mutex(&psZCBNode->sLock);

        status = eZCB_HandleZcbEvent(psEvent);
        if(status != E_ZCB_OK)
            user_controlbridge_log("Handle Zcb Event Err");
    }
    return;
}



/****************************************************************************
* Function	: ZCB_HandleDeviceLeave
* Description	: 处理设备离开
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void ZCB_HandleDeviceLeave(void *pvUser, uint16_t u16Length, void *pvMessage)
{
#pragma pack(1)
    struct _tsDeviceLeave
    {
        uint64_t    u64IEEEAddress;
        uint8_t     bRejoin;
    }*psMessage = (struct _tsDeviceLeave *)pvMessage;
#pragma pack()
    //psMessage->u64IEEEAddress   = be64toh(psMessage->u64IEEEAddress);
    psMessage->u64IEEEAddress   = ntoh64(psMessage->u64IEEEAddress);

    user_controlbridge_log("Device Left, Address 0x%016llX, rejoining: %d\n",
                           (unsigned long long int)psMessage->u64IEEEAddress,
                           psMessage->bRejoin
                          );

    tsZcbEvent *psEvent = malloc(sizeof(tsZcbEvent));
    if (!psEvent)
    {
        user_controlbridge_log("Memory allocation failure allocating event");
        return;
    }

    psEvent->eEvent                                 = E_ZCB_EVENT_DEVICE_LEFT;
    psEvent->uData.sDeviceLeft.u64IEEEAddress       = psMessage->u64IEEEAddress;
    psEvent->uData.sDeviceLeft.bRejoin              = psMessage->bRejoin;

    teZcbStatus status ;
    status = eZCB_HandleZcbEvent(psEvent);
    if(status != E_ZCB_OK)
        user_controlbridge_log("Handle Zcb Event err");

    //    if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != TRUE)
    //    {
    //        user_controlbridge_log("Error queue'ing event\n");
    //        free(psEvent);
    //    }
    //    else
    //    {
    //        user_controlbridge_log("Device leave queued\n");
    //    }
    return;
}


/****************************************************************************
* Function	: ZCB_HandleMatchDescriptorResponse
* Description	: 处理匹配描述符响应
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void ZCB_HandleMatchDescriptorResponse(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    tsZCB_Node *psZCBNode;
#pragma pack(1)
    struct _tMatchDescriptorResponse
    {
        uint8_t     u8SequenceNo;
        uint8_t     u8Status;
        uint16_t    u16ShortAddress;
        uint8_t     u8NumEndpoints;
        uint8_t     au8Endpoints[255];
    }*psMatchDescriptorResponse = (struct _tMatchDescriptorResponse *)pvMessage;
#pragma pack()
    tsZcbEvent *psEvent = malloc(sizeof(tsZcbEvent));
    if (!psEvent)
    {
        user_controlbridge_log("Memory allocation failure allocating event");
        return;
    }

    psMatchDescriptorResponse->u16ShortAddress  = ntohs(psMatchDescriptorResponse->u16ShortAddress);

    user_controlbridge_log("Match descriptor request response from node 0x%04X - %d matching endpoints.",
                           psMatchDescriptorResponse->u16ShortAddress,
                           psMatchDescriptorResponse->u8NumEndpoints
                          );
    if (psMatchDescriptorResponse->u8NumEndpoints)
    {
        // Device has matching endpoints
#if DBG_ZCB
        if ((psZCBNode = psZCB_FindNodeShortAddress(psMatchDescriptorResponse->u16ShortAddress)) != NULL)
        {
            user_controlbridge_log("Node rejoined");
            mico_rtos_unlock_mutex(&psZCBNode->sLock);
        }
        else
        {
            user_controlbridge_log("New node");
        }
#endif

        if (eZCB_AddNode(psMatchDescriptorResponse->u16ShortAddress, 0, 0, 0, &psZCBNode) == E_ZCB_OK)
        {
            int i;
            for (i = 0; i < psMatchDescriptorResponse->u8NumEndpoints; i++)
            {
                /* Add an endpoint to the device for each response in the match descriptor response */
                if (eZCB_NodeAddEndpoint(psZCBNode, psMatchDescriptorResponse->au8Endpoints[i], 0, NULL) != E_ZCB_OK)
                {
                    return;
                }
            }

            psEvent->eEvent                                 = E_ZCB_EVENT_DEVICE_MATCH;
            psEvent->uData.sDeviceMatch.u16ShortAddress     = psZCBNode->u16ShortAddress;

            mico_rtos_unlock_mutex(&psZCBNode->sLock);
            user_controlbridge_log("Queue new node event");

            teZcbStatus status ;
            status = eZCB_HandleZcbEvent(psEvent);
            if(status != E_ZCB_OK)
                user_controlbridge_log("Handle Zcb Event err");
            //            if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != TRUE)
            //            {
            //                user_controlbridge_log("Error queue'ing event\n");
            //                free(psEvent);
            //            }
        }
    }
    return;
}




/****************************************************************************
* Function	: ZCB_HandleAttributeReport
* Description	: 处理属性上报
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void ZCB_HandleAttributeReport(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    teZcbStatus eStatus = E_ZCB_ERROR;

#pragma pack(1)
    struct _tsAttributeReport
    {
        uint8_t     u8SequenceNo;
        uint16_t    u16ShortAddress;
        uint8_t     u8Endpoint;
        uint16_t    u16ClusterID;
        uint16_t    u16AttributeID;
        uint8_t     u8AttributeStatus;
        uint8_t     u8Type;
        union
        {
            uint8_t     u8Data;
            uint16_t    u16Data;
            uint32_t    u32Data;
            uint64_t    u64Data;
        } uData;
    }*psMessage = (struct _tsAttributeReport *)pvMessage;
#pragma pack()
    psMessage->u16ShortAddress  = ntohs(psMessage->u16ShortAddress);
    psMessage->u16ClusterID     = ntohs(psMessage->u16ClusterID);
    psMessage->u16AttributeID   = ntohs(psMessage->u16AttributeID);

    user_controlbridge_log("Attribute report from 0x%04X - Endpoint %d, cluster 0x%04X, attribute %d.\n",
                           psMessage->u16ShortAddress,
                           psMessage->u8Endpoint,
                           psMessage->u16ClusterID,
                           psMessage->u16AttributeID
                          );

    tsZcbEvent *psEvent = malloc(sizeof(tsZcbEvent));
    if (!psEvent)
    {
        user_controlbridge_log("Memory allocation failure allocating event");
        return;
    }

    psEvent->eEvent                                     = E_ZCB_EVENT_ATTRIBUTE_REPORT;
    psEvent->uData.sAttributeReport.u16ShortAddress     = psMessage->u16ShortAddress;
    psEvent->uData.sAttributeReport.u8Endpoint          = psMessage->u8Endpoint;
    psEvent->uData.sAttributeReport.u16ClusterID        = psMessage->u16ClusterID;
    psEvent->uData.sAttributeReport.u16AttributeID      = psMessage->u16AttributeID;
    psEvent->uData.sAttributeReport.eType               = psMessage->u8Type;

    switch(psMessage->u8Type)
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        case(E_ZCL_OSTRING):
        case(E_ZCL_CSTRING):
            psEvent->uData.sAttributeReport.uData.u8Data = psMessage->uData.u8Data;
            eStatus = E_ZCB_OK;
            break;

        case(E_ZCL_LOSTRING):
        case(E_ZCL_LCSTRING):
        case(E_ZCL_STRUCT):
        case(E_ZCL_INT16):
        case(E_ZCL_UINT16):
        case(E_ZCL_ENUM16):
        case(E_ZCL_CLUSTER_ID):
        case(E_ZCL_ATTRIBUTE_ID):
            psEvent->uData.sAttributeReport.uData.u16Data = ntohs(psMessage->uData.u16Data);
            eStatus = E_ZCB_OK;
            break;

        case(E_ZCL_UINT24):
        case(E_ZCL_UINT32):
        case(E_ZCL_TOD):
        case(E_ZCL_DATE):
        case(E_ZCL_UTCT):
        case(E_ZCL_BACNET_OID):
            psEvent->uData.sAttributeReport.uData.u32Data = ntohl(psMessage->uData.u32Data);
            eStatus = E_ZCB_OK;
            break;

        case(E_ZCL_UINT40):
        case(E_ZCL_UINT48):
        case(E_ZCL_UINT56):
        case(E_ZCL_UINT64):
        case(E_ZCL_IEEE_ADDR):
            //psEvent->uData.sAttributeReport.uData.u64Data = be64toh(psMessage->uData.u64Data);
            psEvent->uData.sAttributeReport.uData.u64Data = ntoh64(psMessage->uData.u64Data);
            eStatus = E_ZCB_OK;
            break;

        default:
            user_controlbridge_log("Unknown attribute data type (%d) received from node 0x%04X", psMessage->u8Type, psMessage->u16ShortAddress);
            break;
    }

    if (eStatus == E_ZCB_OK)
    {
        teZcbStatus status ;
        status = eZCB_HandleZcbEvent(psEvent);
        if(status != E_ZCB_OK)
            user_controlbridge_log("Handle Zcb Event err");
        //        if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) != TRUE)
        //        {
        //            user_controlbridge_log("Error queue'ing event\n");
        //            free(psEvent);
        //        }
    }
    else
    {
        free(psEvent);
    }
    return;
}

/****************************************************************************
* Function	: eZCB_ConfigureControlBridge
* Description	: 配置 ControlBridge
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_ConfigureControlBridge(void)
{
#define CONFIGURATION_INTERVAL 50
    /* Set up configuration */
    switch (eStartMode)
    {
        case(E_START_COORDINATOR):
            user_controlbridge_log("Starting control bridge as HA coordinator");
            eZCB_SetDeviceType(E_MODE_COORDINATOR);		//设置设备类型为 coordinator
            mico_thread_msleep(CONFIGURATION_INTERVAL);

            eZCB_SetChannelMask(eChannel);				//设置信道
            mico_thread_msleep(CONFIGURATION_INTERVAL);

            eZCB_SetExtendedPANID(u64PanID);			//设置扩展 PAN ID
            mico_thread_msleep(CONFIGURATION_INTERVAL);

            eZCB_StartNetwork();						//启动网络
            mico_thread_msleep(500);

            eZCB_SetPermitJoining(180);					//设置允许加入网络180s
            mico_thread_msleep(CONFIGURATION_INTERVAL);
            break;

        case (E_START_ROUTER):
            user_controlbridge_log("Starting control bridge as HA compatible router");
            eZCB_SetDeviceType(E_MODE_HA_COMPATABILITY);
            mico_thread_msleep(CONFIGURATION_INTERVAL);

            eZCB_SetChannelMask(eChannel);
            mico_thread_msleep(CONFIGURATION_INTERVAL);

            eZCB_SetExtendedPANID(u64PanID);
            mico_thread_msleep(CONFIGURATION_INTERVAL);

            eZCB_StartNetwork();
            mico_thread_msleep(CONFIGURATION_INTERVAL);
            break;

        case (E_START_TOUCHLINK):
            user_controlbridge_log("Starting control bridge as ZLL router");
            eZCB_SetDeviceType(E_MODE_ROUTER);
            mico_thread_msleep(CONFIGURATION_INTERVAL);

            eZCB_SetChannelMask(eChannel);
            mico_thread_msleep(CONFIGURATION_INTERVAL);

            eZCB_SetExtendedPANID(u64PanID);
            mico_thread_msleep(CONFIGURATION_INTERVAL);

            eZCB_StartNetwork();
            mico_thread_msleep(CONFIGURATION_INTERVAL);
            break;

        default:
            user_controlbridge_log("Unknown module mode\n");
            return E_ZCB_ERROR;
    }

    return E_ZCB_OK;
}


/****************************************************************************
* Function	: eZCB_HandleZcbEvent
* Description	:处理 Zcb 事件
* Input Para	:psEvent事件
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_HandleZcbEvent(tsZcbEvent *psEvent)
{

    switch (psEvent->eEvent)
    {
        case (E_ZCB_EVENT_NETWORK_JOINED):
        case (E_ZCB_EVENT_NETWORK_FORMED):		//加入网络 or 组建网络
            {
                tsZCB_Node *psZcbNode = psZCB_FindNodeControlBridge();	//获得 ControlBridge 节点
                //tsNode *psJIPNode;

                if (!psZcbNode)
                {
                    user_controlbridge_log("Could not find control bridge!\n");
                    break;
                }

                //psJIPNode = psBR_FindJIPNode(psZcbNode);
                //if (psJIPNode)
                //{
                //    user_controlbridge_log("Node 0x%04X is already in the JIP network\n", psZcbNode->u16ShortAddress);
                //    mico_rtos_unlock_mutex(psJIPNode);
                //    mico_rtos_unlock_mutex(&psZcbNode->sLock);
                //    break;
                //}

                // Node joined event
                //eBR_NodeJoined(psZcbNode);

                user_controlbridge_log("Network started\n");

                mico_rtos_unlock_mutex(&psZcbNode->sLock);
                break;
            }

        case (E_ZCB_EVENT_DEVICE_ANNOUNCE):		//device  announce
            {
                tsZCB_Node *psZcbNode = psZCB_FindNodeShortAddress(psEvent->uData.sDeviceAnnounce.u16ShortAddress);
                int i;

                if (!psZcbNode)
                {
                    user_controlbridge_log("Could not find new node!\n");
                    break;
                }


                if (psZcbNode->u32NumEndpoints > 0)
                {
                    user_controlbridge_log("Endpoints of device 0x%04X already known\n", psZcbNode->u16ShortAddress);

                    /* Re-raise event as a device match if we already have the endpoints,
                     * so we can make sure that a JIP device exists for it.
                     */
                    psEvent->eEvent = E_ZCB_EVENT_DEVICE_MATCH;
                    psEvent->uData.sDeviceMatch.u16ShortAddress = psEvent->uData.sDeviceAnnounce.u16ShortAddress;

                    teZcbStatus status ;
                    status = eZCB_HandleZcbEvent(psEvent);
                    if(status != E_ZCB_OK)
                        user_controlbridge_log("Handle Zcb Event err");

                    //if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) == E_UTILS_OK)
                    //{
                    //    psEvent = NULL; /* Prevent event being free'd */
                    //}
                    mico_rtos_unlock_mutex(&psZcbNode->sLock);
                    break;
                }

                user_controlbridge_log("New device 0x%04X", psZcbNode->u16ShortAddress);

                /* Initiate discovery of device */

                //uint16_t au16Profile[] = { E_ZB_PROFILEID_HA, E_ZB_PROFILEID_ZLL };
                uint16_t au16Profile[] = { E_ZB_PROFILEID_HA};
                //uint16_t au16Cluster[] = { E_ZB_CLUSTERID_ONOFF, E_ZB_CLUSTERID_THERMOSTAT  };
                uint16_t au16Cluster[] = { E_ZB_CLUSTERID_ONOFF};

                for (i = 0 ; i < (sizeof(au16Profile)/sizeof(uint16_t)); i++)
                {
                    /* Send match descriptor request */
                    if (eZCB_MatchDescriptorRequest(psZcbNode->u16ShortAddress, au16Profile[i],
                                                    sizeof(au16Cluster) / sizeof(uint16_t), au16Cluster,
                                                    0, NULL, NULL) != E_ZCB_OK)
                    {
                        user_controlbridge_log("Error sending match descriptor request");
                    }
                }
                mico_rtos_unlock_mutex(&psZcbNode->sLock);

                /* Slow down match descriptor requests */
                mico_thread_sleep(1);
                break;
            }

        case (E_ZCB_EVENT_DEVICE_LEFT):
            {
                tsZCB_Node *psZcbNode = psZCB_FindNodeIEEEAddress(psEvent->uData.sDeviceLeft.u64IEEEAddress);

                if (!psZcbNode)
                {
                    user_controlbridge_log("Could not find leaving node!\n");
                    break;
                }

                if (!psEvent->uData.sDeviceLeft.bRejoin)
                {
                    // Device is not going to rejoin so remove it immediately.
                    // If it fails to rejoin it will be aged out via the usual mechanism.
                    //if (eBR_NodeLeft(psZcbNode) != E_BR_OK)
                    //{
                    //    user_controlbridge_log("Error removing node from JIP\n");
                    //}

                    if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                    {
                        user_controlbridge_log("Error removing node from ZCB\n");
                    }
                }
                break;
            }

        case (E_ZCB_EVENT_DEVICE_MATCH):
            {
                int i;
                teZcbStatus eStatus;
                tsZCB_Node *psZcbNode = psZCB_FindNodeShortAddress(psEvent->uData.sDeviceMatch.u16ShortAddress);
                //tsNode *psJIPNode;

                if (!psZcbNode)
                {
                    user_controlbridge_log("Could not find new node!\n");
                    break;
                }


                //psJIPNode = psBR_FindJIPNode(psZcbNode);
                //if (psJIPNode)
                //{
                //    user_controlbridge_log("Node 0x%04X is already in the JIP network\n", psZcbNode->u16ShortAddress);
                //    eJIP_UnlockNode(psJIPNode);
                //}
                //else
                {
                    if (!psZcbNode->u64IEEEAddress)
                    {
                        user_controlbridge_log("New node 0x%04X, requesting IEEE address\n", psZcbNode->u16ShortAddress);
                        if ((eZCB_NodeDescriptorRequest(psZcbNode) != E_ZCB_OK) || (eZCB_IEEEAddressRequest(psZcbNode) != E_ZCB_OK))
                        {
#if 0
                            if (iBR_DeviceTimedOut(psZcbNode))
                            {
                                user_controlbridge_log("Zigbee node 0x%04X removed from network (no response to IEEE Address).\n", psZcbNode->u16ShortAddress);
                                if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                                {
                                    user_controlbridge_log("Error removing node from ZCB\n");
                                }
                            }
                            else
                            {
                                user_controlbridge_log("Error retrieving IEEE Address of node 0x%04X - requeue\n", psZcbNode->u16ShortAddress);
                                if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) == E_UTILS_OK)
                                {
                                    eUtils_LockUnlock(&psZcbNode->sLock);
                                    psEvent = NULL; /* Prevent event being free'd */
                                }
                                else
                                {
                                    /* Failed to re-queue the event - discard node. */
                                    user_controlbridge_log("Event queue is full - removing node\n");
                                    if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                                    {
                                        user_controlbridge_log("Error removing node from ZCB\n");
                                    }
                                }
                            }
#endif
                            break;
                        }
                    }

                    user_controlbridge_log("New device, short address 0x%04X, matching requested clusters", psZcbNode->u16ShortAddress);

                    for (i = 0; i < psZcbNode->u32NumEndpoints; i++)
                    {
                        if (psZcbNode->pasEndpoints[i].u16ProfileID == 0)
                        {

                            user_controlbridge_log("Requesting new endpoint simple descriptor");
                            if (eZCB_SimpleDescriptorRequest(psZcbNode, psZcbNode->pasEndpoints[i].u8Endpoint) != E_ZCB_OK)
                            {
#if 0
                                if (iBR_DeviceTimedOut(psZcbNode))
                                {
                                    user_controlbridge_log("Zigbee node 0x%04X removed from network (No response to simple descriptor request).\n", psZcbNode->u16ShortAddress);
                                    if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                                    {
                                        user_controlbridge_log("Error removing node from ZCB\n");
                                    }
                                }
                                else
                                {
                                    user_controlbridge_log("Failed to read endpoint simple descriptor - requeue\n");
                                    if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) == E_UTILS_OK)
                                    {
                                        eUtils_LockUnlock(&psZcbNode->sLock);
                                        psEvent = NULL; /* Prevent event being free'd */
                                    }
                                    else
                                    {
                                        /* Failed to re-queue the event - discard node. */
                                        user_controlbridge_log("Event queue is full - removing node\n");
                                        if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                                        {
                                            user_controlbridge_log("Error removing node from ZCB\n");
                                        }
                                    }
                                }
#endif
                                /* Get out of the loop and case without adding groups or joining to BR */
                                goto E_ZCB_EVENT_DEVICE_MATCH_done;
                            }
                        }
                    }


                    /* Set up bulb groups */
                    eStatus = eZCB_AddGroupMembership(psZcbNode, 0xf00f);
                    if ((eStatus != E_ZCB_OK) &&
                            (eStatus != E_ZCB_DUPLICATE_EXISTS) &&
                            (eStatus != E_ZCB_UNKNOWN_CLUSTER))
                    {
                        user_controlbridge_log("add Group err");
#if 0
                        if (iBR_DeviceTimedOut(psZcbNode))
                        {
                            user_controlbridge_log("Zigbee node 0x%04X removed from network (No response to add group request).\n", psZcbNode->u16ShortAddress);
                            if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                            {
                                user_controlbridge_log("Error removing node from ZCB\n");
                            }
                        }
                        else
                        {
                            user_controlbridge_log("Failed to add group - requeue\n");
                            /* requeue event */
                            if (eUtils_QueueQueue(&sZcbEventQueue, psEvent) == E_UTILS_OK)
                            {
                                eUtils_LockUnlock(&psZcbNode->sLock);
                                psEvent = NULL; /* Prevent event being free'd */
                            }
                            else
                            {
                                /* Failed to re-queue the event - discard node. */
                                user_controlbridge_log("Event queue is full - removing node\n");
                                if (eZCB_RemoveNode(psZcbNode) != E_ZCB_OK)
                                {
                                    user_controlbridge_log("Error removing node from ZCB\n");
                                }
                            }
                        }
#endif
                        /* Get out of the loop and case without adding groups or joining to BR */
                        goto E_ZCB_EVENT_DEVICE_MATCH_done;
                    }
                }

                // Node joined event.
                //if (!psJIPNode)
                //{
                //    user_controlbridge_log("Adding node 0x%04X to border router\n", psZcbNode->u16ShortAddress);
                //    eBR_NodeJoined(psZcbNode);
                //}
                mico_rtos_unlock_mutex(&psZcbNode->sLock);
E_ZCB_EVENT_DEVICE_MATCH_done:
                break;
            }

        case (E_ZCB_EVENT_ATTRIBUTE_REPORT):
            {
                tsZCB_Node *psZcbNode = psZCB_FindNodeShortAddress(psEvent->uData.sAttributeReport.u16ShortAddress);
                //tsNode *psJIPNode;

                if (!psZcbNode)
                {
                    user_controlbridge_log("Could not find node matching attribute report source 0x%04X!\n", psEvent->uData.sAttributeReport.u16ShortAddress);
                    break;
                }
#if 0
                psJIPNode = psBR_FindJIPNode(psZcbNode);
                if (!psJIPNode)
                {
                    user_controlbridge_log("No JIP device for Zigbee node 0x%04X\n", psZcbNode->u16ShortAddress);
                }
                else
                {
                    tsDeviceIDMap *psDeviceIDMap = asDeviceIDMap;
                    while (((psDeviceIDMap->u16ZigbeeDeviceID != 0) &&
                            (psDeviceIDMap->u32JIPDeviceID != 0) &&
                            (psDeviceIDMap->prInitaliseRoutine != NULL)))
                    {
                        if (psDeviceIDMap->u16ZigbeeDeviceID == psZcbNode->u16DeviceID)
                        {
                            user_controlbridge_log("Found JIP device type 0x%08X for ZB Device type 0x%04X\n",
                                                   psDeviceIDMap->u32JIPDeviceID, psDeviceIDMap->u16ZigbeeDeviceID);
                            if (psDeviceIDMap->prAttributeUpdateRoutine)
                            {
                                user_controlbridge_log("Calling JIP attribute update routine\n");
                                psDeviceIDMap->prAttributeUpdateRoutine(psZcbNode, psJIPNode,
                                                                        psEvent->uData.sAttributeReport.u16ClusterID,
                                                                        psEvent->uData.sAttributeReport.u16AttributeID,
                                                                        psEvent->uData.sAttributeReport.eType,
                                                                        psEvent->uData.sAttributeReport.uData);
                            }
                        }

                        /* Next device map */
                        psDeviceIDMap++;
                    }
                    eJIP_UnlockNode(psJIPNode);
                }
#endif
                mico_rtos_unlock_mutex(&psZcbNode->sLock);
                break;
            }

        default:
            user_controlbridge_log("Unhandled event code\n");
            break;
    }
    if(psEvent != NULL)
        free(psEvent);

    return E_ZCB_OK;
}
/* PDM Messages */



/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
