
/***************************************************************************************
****************************************************************************************
* FILE		: ZigBeeNetwork.c
* Description	:
*
* Copyright (c) 2015 by XXX. All Rights Reserved.
*
* History:
* Version		Name       		Date			Description
0.1		XXX	2015/07/15	Initial Version

****************************************************************************************
****************************************************************************************/
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ZigbeeControlBridge.h"
#include "ZigbeeConstant.h"
#include "ZigbeeNetwork.h"
#include "ZigBeeSerialLink.h"



#define user_ZBNetwork_log(M, ...) custom_log("ZigBee_Network", M, ##__VA_ARGS__)
#define user_ZBNetwork_log_trace() custom_log_trace("ZigBee_Network")


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_ZBNETWORK 0

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


tsZCB_Network sZCB_Network;


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/



/****************************************************************************
* Function	: DBG_PrintNode
* Description	: Print Node Info
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void DBG_PrintNode(tsZCB_Node *psNode)
{
    int i, j, k;

//    user_ZBNetwork_log("Node Short Address: 0x%04X, IEEE Address: 0x%016llX MAC Capability 0x%02X Device ID 0x%04X",
//                       psNode->u16ShortAddress,
//                       (unsigned long long int)psNode->u64IEEEAddress,
//                       psNode->u8MacCapability,
//                       psNode->u16DeviceID
//                      );
    for (i = 0; i < psNode->u32NumEndpoints; i++)
    {
        const char *pcProfileName = NULL;
        tsZCB_NodeEndpoint  *psEndpoint = &psNode->pasEndpoints[i];

        switch (psEndpoint->u16ProfileID)
        {
            case (0x0104):
                pcProfileName = "ZHA";
                break;
            case (0xC05E):
                pcProfileName = "ZLL";
                break;
            default:
                pcProfileName = "Unknown";
                break;
        }

//        user_ZBNetwork_log("  Endpoint %d - Profile 0x%04X (%s)\n",
//                           psEndpoint->u8Endpoint,
//                           psEndpoint->u16ProfileID,
//                           pcProfileName
//                          );

#if 0 
        for (j = 0; j < psEndpoint->u32NumClusters; j++)
        {
            tsZCB_NodeCluster *psCluster = &psEndpoint->pasClusters[j];
            user_ZBNetwork_log("    Cluster ID 0x%04X", psCluster->u16ClusterID);

            user_ZBNetwork_log("      Attributes:");
            for (k = 0; k < psCluster->u32NumAttributes; k++)
            {
                user_ZBNetwork_log("        Attribute ID 0x%04X", psCluster->pau16Attributes[k]);
            }

            user_ZBNetwork_log("      Commands:");
            for (k = 0; k < psCluster->u32NumCommands; k++)
            {
                user_ZBNetwork_log("        Command ID 0x%02X", psCluster->pau8Commands[k]);
            }
        }
#endif
    }
}



/****************************************************************************
* Function	: eZCB_AddNode
* Description	: Add Node
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_AddNode(uint16_t u16ShortAddress, uint64_t u64IEEEAddress, uint16_t u16DeviceID, uint8_t u8MacCapability, tsZCB_Node **ppsZCBNode)
{
    teZcbStatus eStatus = E_ZCB_OK;
    tsZCB_Node *psZCBNode = &sZCB_Network.sNodes;

    mico_rtos_lock_mutex(&sZCB_Network.sLock);

    while (psZCBNode->psNext)
    {
        if (u64IEEEAddress)
        {
            if (psZCBNode->psNext->u64IEEEAddress == u64IEEEAddress)
            {
                user_ZBNetwork_log("IEEE address already in network - update short address\n");
                mico_rtos_lock_mutex(&psZCBNode->psNext->sLock);
                psZCBNode->psNext->u16ShortAddress = u16ShortAddress;

                if (ppsZCBNode)
                {
                    *ppsZCBNode = psZCBNode->psNext;
                }
                else
                {
                    mico_rtos_unlock_mutex(&psZCBNode->psNext->sLock);
                }
                goto done;
            }
            if (psZCBNode->psNext->u16ShortAddress == u16ShortAddress)
            {
                user_ZBNetwork_log("Short address already in network - update IEEE address\n");
                mico_rtos_lock_mutex(&psZCBNode->psNext->sLock);
                psZCBNode->psNext->u64IEEEAddress = u64IEEEAddress;

                if (ppsZCBNode)
                {
                    *ppsZCBNode = psZCBNode->psNext;
                }
                else
                {
                    mico_rtos_unlock_mutex(&psZCBNode->psNext->sLock);
                }
                goto done;
            }
        }
        else
        {
            if (psZCBNode->psNext->u16ShortAddress == u16ShortAddress)
            {
                user_ZBNetwork_log("Short address already in network\n");
                mico_rtos_lock_mutex(&psZCBNode->psNext->sLock);

                if (ppsZCBNode)
                {
                    *ppsZCBNode = psZCBNode->psNext;
                }
                else
                {
                    mico_rtos_unlock_mutex(&psZCBNode->psNext->sLock);
                }
                goto done;
            }

        }
        psZCBNode = psZCBNode->psNext;
    }

    psZCBNode->psNext = malloc(sizeof(tsZCB_Node));

    if (!psZCBNode->psNext)
    {
        user_ZBNetwork_log("Memory allocation failure allocating node");
        eStatus = E_ZCB_ERROR_NO_MEM;
        goto done;
    }

    memset(psZCBNode->psNext, 0, sizeof(tsZCB_Node));

    /* Got to end of list without finding existing node - add it at the end of the list */
    mico_rtos_init_mutex(&psZCBNode->psNext->sLock);
    psZCBNode->psNext->u16ShortAddress  = u16ShortAddress;
    psZCBNode->psNext->u64IEEEAddress   = u64IEEEAddress;
    psZCBNode->psNext->u8MacCapability  = u8MacCapability;
    psZCBNode->psNext->u16DeviceID      = u16DeviceID;

    user_ZBNetwork_log("Created new Node\n");
    DBG_PrintNode(psZCBNode->psNext);

    if (ppsZCBNode)
    {
        mico_rtos_lock_mutex(&psZCBNode->psNext->sLock);
        *ppsZCBNode = psZCBNode->psNext;
    }

done:
    mico_rtos_unlock_mutex(&sZCB_Network.sLock);
    return eStatus;
}

/****************************************************************************
* Function	: eZCB_RemoveNode
* Description	: Remove Node
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_RemoveNode(tsZCB_Node *psZCBNode)
{
    teZcbStatus eStatus = E_ZCB_ERROR;
    tsZCB_Node *psZCBCurrentNode = &sZCB_Network.sNodes;
    int iNodeFreeable = 0;

    /* lock the list mutex and node mutex in the same order as everywhere else to avoid deadlock */

    //mico_rtos_unlock_mutex(&psZCBNode->sLock);

    //mico_rtos_lock_mutex(&sZCB_Network.sLock);

    //mico_rtos_lock_mutex(&psZCBNode->sLock);

    if (psZCBNode == &sZCB_Network.sNodes)
    {
        eStatus = E_ZCB_OK;
        iNodeFreeable = 0;
    }
    else
    {
        while (psZCBCurrentNode)
        {
            if (psZCBCurrentNode->psNext == psZCBNode)
            {
                user_ZBNetwork_log("Found node to remove\n");
                DBG_PrintNode(psZCBNode);

                psZCBCurrentNode->psNext = psZCBCurrentNode->psNext->psNext;
                eStatus = E_ZCB_OK;
                iNodeFreeable = 1;
                break;
            }
            psZCBCurrentNode = psZCBCurrentNode->psNext;
        }
    }

    if (eStatus == E_ZCB_OK)
    {
        int i, j;
        for (i = 0; i < psZCBNode->u32NumEndpoints; i++)
        {
            user_ZBNetwork_log("Free endpoint %d\n", psZCBNode->pasEndpoints[i].u8Endpoint);
            if (psZCBNode->pasEndpoints[i].pasClusters)
            {
                for (j = 0; j < psZCBNode->pasEndpoints[i].u32NumClusters; j++)
                {
                    user_ZBNetwork_log("Free cluster 0x%04X\n", psZCBNode->pasEndpoints[i].pasClusters[j].u16ClusterID);
                    free(psZCBNode->pasEndpoints[i].pasClusters[j].pau16Attributes);
                    free(psZCBNode->pasEndpoints[i].pasClusters[j].pau8Commands);
                }
                free(psZCBNode->pasEndpoints[i].pasClusters);

            }
        }
        free(psZCBNode->pasEndpoints);

        free(psZCBNode->pau16Groups);

        /* Unlock the node first so that it may be free'd */
        //mico_rtos_unlock_mutex(&psZCBNode->sLock);
        //mico_rtos_deinit_mutex(&psZCBNode->sLock);
        if (iNodeFreeable)
        {
            free(psZCBNode);
        }
    }
    //mico_rtos_unlock_mutex(&sZCB_Network.sLock);
    return eStatus;
}



/****************************************************************************
* Function	: psZCB_FindNodeIEEEAddress
* Description	: Find Node
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
tsZCB_Node *psZCB_FindNodeIEEEAddress(uint64_t u64IEEEAddress)
{
    tsZCB_Node *psZCBNode = &sZCB_Network.sNodes;

    mico_rtos_lock_mutex(&sZCB_Network.sLock);

    while (psZCBNode)
    {
        if (psZCBNode->u64IEEEAddress == u64IEEEAddress)
        {
            int iLockAttempts = 0;

            user_ZBNetwork_log("IEEE address 0x%016llX found in network\n", (unsigned long long int)u64IEEEAddress);
            DBG_PrintNode(psZCBNode);

            while (++iLockAttempts < 5)
            {
                if (mico_rtos_lock_mutex(&psZCBNode->sLock) == kNoErr)
                {
                    break;
                }
                else
                {
                    mico_rtos_unlock_mutex(&sZCB_Network.sLock);

                    if (iLockAttempts == 5)
                    {
                        user_ZBNetwork_log("\nError: Could not get lock on node!!");
                        return NULL;
                    }

                    mico_thread_msleep(1000);
                    mico_rtos_lock_mutex(&sZCB_Network.sLock);
                }
            }
            break;
        }
        psZCBNode = psZCBNode->psNext;
    }

    mico_rtos_unlock_mutex(&sZCB_Network.sLock);
    return psZCBNode;
}



/****************************************************************************
* Function	: psZCB_FindNodeShortAddress
* Description	: Find Node
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
tsZCB_Node *psZCB_FindNodeShortAddress(uint16_t u16ShortAddress)
{
    tsZCB_Node *psZCBNode = &sZCB_Network.sNodes;

    mico_rtos_lock_mutex(&sZCB_Network.sLock);

    while (psZCBNode)
    {
        if (psZCBNode->u16ShortAddress == u16ShortAddress)
        {
            int iLockAttempts = 0;

            user_ZBNetwork_log("Short address 0x%04X found in network\n", u16ShortAddress);
            DBG_PrintNode(psZCBNode);

            while (++iLockAttempts < 5)
            {
                if (mico_rtos_lock_mutex(&psZCBNode->sLock) == kNoErr)
                {
                    break;
                }
                else
                {
                    mico_rtos_unlock_mutex(&sZCB_Network.sLock);

                    if (iLockAttempts == 5)
                    {
                        user_ZBNetwork_log("\n\nError: Could not get lock on node!!\n");
                        return NULL;
                    }

                    mico_thread_msleep(1000);
                    mico_rtos_lock_mutex(&sZCB_Network.sLock);
                }
            }
            break;
        }
        psZCBNode = psZCBNode->psNext;
    }

    mico_rtos_unlock_mutex(&sZCB_Network.sLock);
    return psZCBNode;
}



/****************************************************************************
* Function	: psZCB_FindNodeControlBridge
* Description	: Find ControlBridge
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
tsZCB_Node *psZCB_FindNodeControlBridge(void)
{
    tsZCB_Node *psZCBNode = &sZCB_Network.sNodes;
    //mico_rtos_lock_mutex(&psZCBNode->sLock);
    return psZCBNode;
}



/****************************************************************************
* Function	: eZCB_NodeAddEndpoint
* Description	: Node Add Endpoint
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_NodeAddEndpoint(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ProfileID, tsZCB_NodeEndpoint **ppsEndpoint)
{
    tsZCB_NodeEndpoint *psNewEndpoint;
    int i;

    //user_ZBNetwork_log("Add Endpoint %d, profile 0x%04X to node 0x%04X", u8Endpoint, u16ProfileID, psZCBNode->u16ShortAddress);

    for (i = 0; i < psZCBNode->u32NumEndpoints; i++)
    {
        if (psZCBNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            user_ZBNetwork_log("Duplicate Endpoint");
            if (u16ProfileID)
            {
                user_ZBNetwork_log("Set Endpoint %d profile to 0x%04X", u8Endpoint, u16ProfileID);
                psZCBNode->pasEndpoints[i].u16ProfileID = u16ProfileID;
            }
            return E_ZCB_OK;
        }
    }

    //user_ZBNetwork_log("Creating new endpoint %d", u8Endpoint);

    psNewEndpoint = realloc(psZCBNode->pasEndpoints, sizeof(tsZCB_NodeEndpoint) * (psZCBNode->u32NumEndpoints+1));

    if (!psNewEndpoint)
    {
        user_ZBNetwork_log("Memory allocation failure allocating endpoint");
        return E_ZCB_ERROR_NO_MEM;
    }

    psZCBNode->pasEndpoints = psNewEndpoint;

    memset(&psZCBNode->pasEndpoints[psZCBNode->u32NumEndpoints], 0, sizeof(tsZCB_NodeEndpoint));
    psNewEndpoint = &psZCBNode->pasEndpoints[psZCBNode->u32NumEndpoints];
    psZCBNode->u32NumEndpoints++;

    psNewEndpoint->u8Endpoint = u8Endpoint;
    psNewEndpoint->u16ProfileID = u16ProfileID;


    if (ppsEndpoint)
    {
        *ppsEndpoint = psNewEndpoint;
    }

    return E_ZCB_OK;
}



/****************************************************************************
* Function	: eZCB_NodeAddCluster
* Description	: Node Add Cluster
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_NodeAddCluster(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ClusterID)
{
    int i;
    tsZCB_NodeEndpoint *psEndpoint = NULL;
    tsZCB_NodeCluster  *psNewClusters;

    //user_ZBNetwork_log("Node 0x%04X: Add cluster 0x%04X to Endpoint %d", psZCBNode->u16ShortAddress, u16ClusterID, u8Endpoint);

    for (i = 0; i < psZCBNode->u32NumEndpoints; i++)
    {
        if (psZCBNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            psEndpoint = &psZCBNode->pasEndpoints[i];
            break;
        }
    }
    if (!psEndpoint)
    {
        user_ZBNetwork_log("Endpoint not found");
        return E_ZCB_UNKNOWN_ENDPOINT;
    }

    for (i = 0; i < psEndpoint->u32NumClusters; i++)
    {
        if (psEndpoint->pasClusters[i].u16ClusterID == u16ClusterID)
        {
            //user_ZBNetwork_log("Duplicate %d th Cluster ID:%d",i,u16ClusterID);
			
            return E_ZCB_OK;
        }
    }

    psNewClusters = realloc(psEndpoint->pasClusters, sizeof(tsZCB_NodeCluster) * (psEndpoint->u32NumClusters+1));
    if (!psNewClusters)
    {
        user_ZBNetwork_log("Memory allocation failure allocating clusters");
        return E_ZCB_ERROR_NO_MEM;
    }
    psEndpoint->pasClusters = psNewClusters;

    memset(&psEndpoint->pasClusters[psEndpoint->u32NumClusters], 0, sizeof(tsZCB_NodeCluster));
    psEndpoint->pasClusters[psEndpoint->u32NumClusters].u16ClusterID = u16ClusterID;
    psEndpoint->u32NumClusters++;

    return E_ZCB_OK;
}



/****************************************************************************
* Function	: eZCB_NodeAddAttribute
* Description	: Node Add Attribute
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_NodeAddAttribute(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ClusterID, uint16_t u16AttributeID)
{
    int i;
    tsZCB_NodeEndpoint *psEndpoint = NULL;
    tsZCB_NodeCluster  *psCluster = NULL;
    uint16_t *pu16NewAttributeList;

    //user_ZBNetwork_log("Node 0x%04X: Add Attribute 0x%04X to cluster 0x%04X on Endpoint %d\n",
    //                   psZCBNode->u16ShortAddress, u16AttributeID, u16ClusterID, u8Endpoint);

    for (i = 0; i < psZCBNode->u32NumEndpoints; i++)
    {
        if (psZCBNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            psEndpoint = &psZCBNode->pasEndpoints[i];
        }
    }
    if (!psEndpoint)
    {
        user_ZBNetwork_log("Endpoint not found\n");
        return E_ZCB_UNKNOWN_ENDPOINT;
    }

    for (i = 0; i < psEndpoint->u32NumClusters; i++)
    {
        if (psEndpoint->pasClusters[i].u16ClusterID == u16ClusterID)
        {
            psCluster = &psEndpoint->pasClusters[i];
        }
    }
    if (!psCluster)
    {
        user_ZBNetwork_log("Cluster not found\n");
        return E_ZCB_UNKNOWN_CLUSTER;
    }

    for (i = 0; i < psCluster->u32NumAttributes; i++)
    {
        if (psCluster->pau16Attributes[i] == u16AttributeID)
        {
            user_ZBNetwork_log("Duplicate Attribute ID\n");
            return E_ZCB_ERROR;
        }
    }

    pu16NewAttributeList = realloc(psCluster->pau16Attributes, sizeof(uint16_t) * (psCluster->u32NumAttributes + 1));

    if (!pu16NewAttributeList)
    {
        user_ZBNetwork_log("Memory allocation failure allocating attributes");
        return E_ZCB_ERROR_NO_MEM;
    }
    psCluster->pau16Attributes = pu16NewAttributeList;

    psCluster->pau16Attributes[psCluster->u32NumAttributes] = u16AttributeID;
    psCluster->u32NumAttributes++;

    return E_ZCB_OK;
}



/****************************************************************************
* Function	: eZCB_NodeAddCommand
* Description	: Node Add Command
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_NodeAddCommand(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ClusterID, uint8_t u8CommandID)
{
    int i;
    tsZCB_NodeEndpoint *psEndpoint = NULL;
    tsZCB_NodeCluster  *psCluster = NULL;
    uint8_t *pu8NewCommandList;

    //user_ZBNetwork_log("Node 0x%04X: Add Command 0x%02X to cluster 0x%04X on Endpoint %d\n",
    //                   psZCBNode->u16ShortAddress, u8CommandID, u16ClusterID, u8Endpoint);

    for (i = 0; i < psZCBNode->u32NumEndpoints; i++)
    {
        if (psZCBNode->pasEndpoints[i].u8Endpoint == u8Endpoint)
        {
            psEndpoint = &psZCBNode->pasEndpoints[i];
        }
    }
    if (!psEndpoint)
    {
        user_ZBNetwork_log("Endpoint not found\n");
        return E_ZCB_UNKNOWN_ENDPOINT;
    }

    for (i = 0; i < psEndpoint->u32NumClusters; i++)
    {
        if (psEndpoint->pasClusters[i].u16ClusterID == u16ClusterID)
        {
            psCluster = &psEndpoint->pasClusters[i];
        }
    }
    if (!psCluster)
    {
        user_ZBNetwork_log("Cluster not found\n");
        return E_ZCB_UNKNOWN_CLUSTER;
    }

    for (i = 0; i < psCluster->u32NumCommands; i++)
    {
        if (psCluster->pau8Commands[i] == u8CommandID)
        {
            user_ZBNetwork_log("Duplicate Command ID\n");
            return E_ZCB_ERROR;
        }
    }

    pu8NewCommandList = realloc(psCluster->pau8Commands, sizeof(uint8_t) * (psCluster->u32NumCommands + 1));

    if (!pu8NewCommandList)
    {
        user_ZBNetwork_log("Memory allocation failure allocating commands");
        return E_ZCB_ERROR_NO_MEM;
    }
    psCluster->pau8Commands = pu8NewCommandList;

    psCluster->pau8Commands[psCluster->u32NumCommands] = u8CommandID;
    psCluster->u32NumCommands++;

    return E_ZCB_OK;
}



/****************************************************************************
* Function	: psZCB_NodeFindEndpoint
* Description	: Node Find Endpoint
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
tsZCB_NodeEndpoint *psZCB_NodeFindEndpoint(tsZCB_Node *psZCBNode, uint16_t u16ClusterID)
{
    int i, j;
    tsZCB_NodeEndpoint *psEndpoint = NULL;

    user_ZBNetwork_log("Node 0x%04X: Find cluster 0x%04X\n", psZCBNode->u16ShortAddress, u16ClusterID);

    for (i = 0; i < psZCBNode->u32NumEndpoints; i++)
    {
        psEndpoint = &psZCBNode->pasEndpoints[i];

        for (j = 0; j < psEndpoint->u32NumClusters; j++)
        {
            if (psEndpoint->pasClusters[j].u16ClusterID == u16ClusterID)
            {
                user_ZBNetwork_log("Found Cluster ID on Endpoint %d\n", psEndpoint->u8Endpoint);
                return psEndpoint;
            }
        }
    }
    user_ZBNetwork_log("Cluster 0x%04X not found on node 0x%04X\n", u16ClusterID, psZCBNode->u16ShortAddress);
    return NULL;
}



/****************************************************************************
* Function	: eZCB_NodeClearGroups
* Description	: Node Clear Groups
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_NodeClearGroups(tsZCB_Node *psZCBNode)
{
    user_ZBNetwork_log("Node 0x%04X: Clear groups\n", psZCBNode->u16ShortAddress);

    if (psZCBNode->pau16Groups)
    {
        free(psZCBNode->pau16Groups);
        psZCBNode->pau16Groups = NULL;
    }

    psZCBNode->u32NumGroups = 0;
    return E_ZCB_OK;
}



/****************************************************************************
* Function	: eZCB_NodeAddGroup
* Description	: Node Add Group
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus eZCB_NodeAddGroup(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress)
{
    uint16_t *pu16NewGroups;
    int i;

    user_ZBNetwork_log("Node 0x%04X: Add group 0x%04X\n", psZCBNode->u16ShortAddress, u16GroupAddress);

    if (psZCBNode->pau16Groups)
    {
        for (i = 0; i < psZCBNode->u32NumGroups; i++)
        {
            if (psZCBNode->pau16Groups[i] == u16GroupAddress)
            {
                user_ZBNetwork_log("Node is already in group 0x%04X\n", u16GroupAddress);
                return E_ZCB_OK;
            }
        }
    }

    pu16NewGroups = realloc(psZCBNode->pau16Groups, sizeof(uint16_t) * (psZCBNode->u32NumGroups + 1));

    if (!pu16NewGroups)
    {
        user_ZBNetwork_log("Memory allocation failure allocating groups");
        return E_ZCB_ERROR_NO_MEM;
    }

    psZCBNode->pau16Groups = pu16NewGroups;
    psZCBNode->pau16Groups[psZCBNode->u32NumGroups] = u16GroupAddress;
    psZCBNode->u32NumGroups++;
    return E_ZCB_OK;
}



/****************************************************************************
* Function	: vZCB_NodeUpdateComms
* Description	: Node Update Comms
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void vZCB_NodeUpdateComms(tsZCB_Node *psZCBNode, teZcbStatus eStatus)
{
    if (!psZCBNode)
    {
        return;
    }

    if (eStatus == E_ZCB_OK)
    {
        //gettimeofday(&psZCBNode->sComms.sLastSuccessful, NULL);
        psZCBNode->sComms.u16SequentialFailures = 0;
    }
    else if (eStatus == E_ZCB_COMMS_FAILED)
    {
        psZCBNode->sComms.u16SequentialFailures++;
    }
    else
    {

    }
#if DBG_ZBNETWORK
    {
        time_t nowtime;
        struct tm *nowtm;
        char tmbuf[64], buf[64];

        nowtime = psZCBNode->sComms.sLastSuccessful.tv_sec;
        nowtm = localtime(&nowtime);
        strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
        snprintf(buf, sizeof buf, "%s.%06d", tmbuf, (int)psZCBNode->sComms.sLastSuccessful.tv_usec);
        user_ZBNetwork_log("Node 0x%04X: %d sequential comms failures. Last successful comms at %s\n",
                           psZCBNode->u16ShortAddress, psZCBNode->sComms.u16SequentialFailures, buf);
    }
#endif /* DBG_ZBNETWORK */
}


//tsZCB_Node *psZCB_NodeOldestComms(void)
//{
//    tsZCB_Node *psZCBNode = &sZCB_Network.sNodes;
//    tsZCB_Node *psZCBNodeComms = NULL;
//    struct timeval sNow;
//    uint32_t u32LargestTimeDiff = 0;
//
//    //mico_rtos_lock_mutex(&sZCB_Network.sLock);
//
//    gettimeofday(&sNow, NULL);
//
//    while (psZCBNode->psNext)
//    {
//        uint32_t u32ThisDiff = u32TimevalDiff(&psZCBNode->psNext->sComms.sLastSuccessful, &sNow);
//        if (u32ThisDiff > u32LargestTimeDiff)
//        {
//            u32LargestTimeDiff = u32ThisDiff;
//            mico_log("Node 0x%04X has oldest comms (%dms ago)\n", psZCBNode->psNext->u16ShortAddress, u32ThisDiff);
//            psZCBNodeComms = psZCBNode->psNext;
//        }
//        psZCBNode = psZCBNode->psNext;
//    }
//
//    if (psZCBNodeComms)
//    {
//        //mico_rtos_lock_mutex(&psZCBNodeComms->sLock);
//    }
//
//    //mico_rtos_unlock_mutex(&sZCB_Network.sLock);
//    return psZCBNodeComms;
//}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

//static uint32_t u32TimevalDiff(struct timeval *psStartTime, struct timeval *psFinishTime)
//{
//    uint32_t u32MSec;
//    u32MSec  = ((uint32_t)psFinishTime->tv_sec - (uint32_t)psStartTime->tv_sec) * 1000;
//    if (psStartTime->tv_usec > psFinishTime->tv_usec)
//    {
//        u32MSec += 1000;
//        psStartTime->tv_usec -= 1000000;
//    }
//    u32MSec += ((uint32_t)psFinishTime->tv_usec - (uint32_t)psStartTime->tv_usec) / 1000;
//
//    mico_log("Time difference is %d milliseconds\n", u32MSec);
//    return u32MSec;
//}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

