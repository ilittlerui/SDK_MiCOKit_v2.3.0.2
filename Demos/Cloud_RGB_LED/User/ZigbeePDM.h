/****************************************************************************
*
* MODULE:             Linux Zigbee control bridge interface daemon
*
* COMPONENT:          PDM
*
* REVISION:           $Revision: 43420 $
*
* DATED:              $Date: 2012-06-18 15:13:17 +0100 (Mon, 18 Jun 2012) $
*
* AUTHOR:             Matt Redfearn
*
****************************************************************************
*
* This software is owned by NXP B.V. and/or its supplier and is protected
* under applicable copyright laws. All rights are reserved. We grant You,
* and any third parties, a license to use this software solely and
* exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139].
* You, and any third parties must reproduce the copyright and warranty notice
* and any other legend of ownership on each copy or partial copy of the
* software.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.

* Copyright NXP B.V. 2012. All rights reserved
*
***************************************************************************/

#include <stdint.h>
#include "mico_system.h"
#ifndef __PDM_H__
#define __PDM_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include "ZigBeeControlBridge.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define ZIGBEE_NODE_LENGTH 16
#define ZIGBEE_NODE_MAX 14
#define ZIGBEE_FLASH_HEAD_LEN	4
#define ZIGBEE_NODE_ABS_ADDR(index)  (ZIGBEE_FLASH_HEAD_LEN + index*ZIGBEE_NODE_LENGTH)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

//flash 中最前面几个字节保存的信息
//1.节点是否在网络中 FFFF
//2.节点是否在线     FFFF

typedef struct _tsZCB_NodePDM
{     
    uint32_t            u32IEEEAddressH;                                                        
    uint32_t            u32IEEEAddressL;                                                //8 bytes 长地址
    uint16_t            u16ShortAddress;                                                //2 bytes 短地址
    uint16_t            u16DeviceID;													//2 bytes 设备ID
	uint8_t 			u8NodeStatus;	                        		                //节点状态 2 bytes:保留
    uint8_t             u8MacCapability;                                                //1 byte mac 兼容性
    uint16_t            u16CustomData;                                                  //2 byte 自定义
} tsZCB_NodePDM;		//	16 bytes


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/** Initialise PDM for control bridge */
teZcbStatus ePDM_Init(mico_Context_t* mico_context);
teZcbStatus ePDM_Destory();
//teZcbStatus ePDM_Save();

teZcbStatus ePDM_GetNodeStatus(uint16_t *status);	//获取每个存储空间的状态
teZcbStatus ePDM_DelAllNodes();						//删除所有节点:永久删除
teZcbStatus ePDM_DisplayAllNode();						//显示所有节点
teZcbStatus ePDM_ReadOneNode(tsZCB_NodePDM *,uint8_t index); //从 index 读取一个节点到 ZCB_NodePDM
teZcbStatus ePDM_ReadOneNode1(tsZCB_NodePDM *,uint8_t index);
teZcbStatus ePDM_DelOneNode(uint64_t *pu64IEEEAddress);	//删除一个节点:根据ieee Address
teZcbStatus ePDM_RemoveOneNode(uint8_t index);		//删除一个节点:根据index
teZcbStatus ePDM_WriteOneNode(tsZCB_NodePDM *,uint8_t index);//写入一个节点 ZCB_NodePDM 到 index
teZcbStatus ePDM_SaveOneNode(tsZCB_Node *,uint16_t u16CustomData);				//保存一个节点:根据 ZCB_Node

teZcbStatus ePDM_UpdateNodeAliveStatus(uint16_t u16alive);//更新每个设备alive状态
teZcbStatus ePDM_GetNodeAliveStatus(uint16_t *status);	//获取每个设备Alive的状态
teZcbStatus ePDM_SaveDeviceAnnounce(void *pvMessage);

teZcbStatus ePDM_FindOneNode(uint16_t u16ShortAddress,uint8_t *pu8index);
void vPDM_Test();
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* __SERIAL_H__ */
