
/***************************************************************************************
****************************************************************************************
* FILE		: ZigBeeNetwork.h
* Description	:
*
* Copyright (c) 2015 by XXX. All Rights Reserved.
*
* History:
* Version		Name       		Date			Description
0.1		XXX	2015/07/15	Initial Version

****************************************************************************************
****************************************************************************************/
#ifndef  _ZIGBEENETWORK_H_
#define  _ZIGBEENETWORK_H_


#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <stdint.h>
#include "mico_rtos.h"
#include "ZigBeeControlBridge.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/** Stucture for the Zigbee network */
typedef struct
{
    mico_mutex_t            sLock;              /**< Lock for the node list */

    tsZCB_Node              sNodes;             /**< Linked list of nodes.
        *   The head is the control bridge */
} tsZCB_Network;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

extern tsZCB_Network sZCB_Network;


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

void DBG_PrintNode(tsZCB_Node *psNode);

teZcbStatus eZCB_NodeAddEndpoint(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ProfileID, tsZCB_NodeEndpoint **ppsEndpoint);
teZcbStatus eZCB_NodeAddCluster(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ClusterID);
teZcbStatus eZCB_NodeAddAttribute(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ClusterID, uint16_t u16AttributeID);
teZcbStatus eZCB_NodeAddCommand(tsZCB_Node *psZCBNode, uint8_t u8Endpoint, uint16_t u16ClusterID, uint8_t u8CommandID);

teZcbStatus eZCB_NodeClearGroups(tsZCB_Node *psZCBNode);
teZcbStatus eZCB_NodeAddGroup(tsZCB_Node *psZCBNode, uint16_t u16GroupAddress);

void        vZCB_NodeUpdateComms(tsZCB_Node *psZCBNode, teZcbStatus eStatus);

/** Find the first endpoint on a node that contains a cluster ID.
*  \param psZCBNode        Pointer to node to search
*  \param u16ClusterID     Cluster ID of interest
*  \return A pointer to the endpoint or NULL if cluster ID not found
*/
tsZCB_NodeEndpoint *psZCB_NodeFindEndpoint(tsZCB_Node *psZCBNode, uint16_t u16ClusterID);


void DisplayZCBNetwork();

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /*_ZIGBEENETWORK_H_*/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

