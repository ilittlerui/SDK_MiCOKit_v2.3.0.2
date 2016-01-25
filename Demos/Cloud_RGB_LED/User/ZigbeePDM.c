/****************************************************************************
*
* MODULE:             PDM for control bridge
*
* COMPONENT:          $RCSfile: PDM.c,v $
*
* REVISION:           $Revision: 43420 $
*
* DATED:              $Date: 2012-06-18 15:13:17 +0100 (Mon, 18 Jun 2012) $
*
* AUTHOR:             Lee Mitchell
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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "platform_peripheral.h"
#include "platform_config.h"
#include "mico_rtos.h"
#include "Debug.h"
#include "Common.h"
#include "ZigBeePDM.h"
#include "ZigBeeSerialLink.h"
#include "ZigBeeControlBridge.h"
#include "MicoDriverFlash.h"

#define user_zigbeePDM_log(M, ...) custom_log("Zigbee_PDM", M, ##__VA_ARGS__)
#define user_zigbeePDM_log_trace() custom_log_trace("Zigbee_PDM")
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DBG_PDM 0
#define DBG_SQL 0

//uint8_t PDM_recoverFlag = 0;
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

//static void PDM_HandleAvailableRequest          (void *pvUser, uint16_t u16Length, void *pvMessage);
//static void PDM_HandleLoadRequest               (void *pvUser, uint16_t u16Length, void *pvMessage);
//static void PDM_HandleSaveRequest               (void *pvUser, uint16_t u16Length, void *pvMessage);
//static void PDM_HandleDeleteAllRequest          (void *pvUser, uint16_t u16Length, void *pvMessage);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static mico_mutex_t sLock;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


teZcbStatus ePDM_Init(mico_Context_t* mico_context)
{
    OSStatus err;
    user_zigbeePDM_log("Create PDM lock");
    mico_rtos_init_mutex(&sLock);
    mico_logic_partition_t *zigbeePDM_partition_info;

    //init  MICO_PARTITION_ZIGBEEPDM_TEMP
    err = MicoFlashInitialize(MICO_PARTITION_ZIGBEEPDM_TEMP);
    require_noerr(err, exit);

    // Get Info  MICO_PARTITION_ZIGBEEPDM_TEMP
    zigbeePDM_partition_info = MicoFlashGetInfo(MICO_PARTITION_ZIGBEEPDM_TEMP);

    //Erase MICO_PARTITION_ZIGBEEPDM_TEMP
    //err = MicoFlashErase( MICO_PARTITION_ZIGBEEPDM_TEMP, 0x0, zigbeePDM_partition_info->partition_length);
    //require_noerr(err, exit);

    //uint32_t j = 0;
#if 0
    for(uint8_t i = 0; i<32; i++)
    {
        off_set = i*256;
        MicoFlashWrite(MICO_PARTITION_ZIGBEEPDM_TEMP, &off_set, testData ,3);
    }


    MicoFlashErase(MICO_PARTITION_ZIGBEEPDM_TEMP,0x0,10);


    for(uint8_t i = 0; i<32; i++)
    {
        off_set = i*256;
        MicoFlashRead(MICO_PARTITION_ZIGBEEPDM_TEMP, &off_set, testData1 ,3);
        user_zigbeePDM_log("%d",testData1[0]);

        memset(testData1,0x0,3);
    }
#endif


    //vPDM_Test();

#if 0
    NodePdm_Test.u32NodeStatus  = 0x00;
    NodePdm_Test.u16DeviceID = 0x00;
    NodePdm_Test.u16ShortAddress = 0x1234;
    NodePdm_Test.u32IEEEAddressH = 0x11223344;
    NodePdm_Test.u32IEEEAddressL = 0x55667788;


    user_zigbeePDM_log("sizeof NodePdm_Test is:%d",sizeof(NodePdm_Test));
    //uint8_t i = 0;
    uint32_t dest_offset = 0;

    mico_rtos_lock_mutex(&sLock);

    //init  MICO_PARTITION_ZIGBEEPDM_TEMP
    err = MicoFlashInitialize(MICO_PARTITION_ZIGBEEPDM_TEMP);
    require_noerr(err, exit);

    // Get Info  MICO_PARTITION_ZIGBEEPDM_TEMP
    zigbeePDM_partition_info = MicoFlashGetInfo(MICO_PARTITION_ZIGBEEPDM_TEMP);
    user_zigbeePDM_log("ZigBee PDM Partition info:start_addr:%x ,length:%x",zigbeePDM_partition_info->partition_start_addr,zigbeePDM_partition_info->partition_length);

    //Erase MICO_PARTITION_ZIGBEEPDM_TEMP
    err = MicoFlashErase( MICO_PARTITION_ZIGBEEPDM_TEMP, 0x0, zigbeePDM_partition_info->partition_length);
    require_noerr(err, exit);


    mico_thread_msleep(100);	//sleep

    dest_offset = 0;
    //Write MICO_PARTITION_ZIGBEEPDM_TEMP							  (uint8_t *)(&NodePdm_Test)
    err = MicoFlashWrite(MICO_PARTITION_ZIGBEEPDM_TEMP, &dest_offset, (uint8_t *)(&NodePdm_Test), sizeof(NodePdm_Test));
    require_noerr(err, exit);

    memset(&NodePdm_Test,0x0,sizeof(NodePdm_Test));
    //Read
    dest_offset = 0;
    err = MicoFlashRead(MICO_PARTITION_ZIGBEEPDM_TEMP, &dest_offset, (uint8_t *)(&NodePdm_Test), sizeof(NodePdm_Test));
    require_noerr(err, exit);

    user_zigbeePDM_log("Node is DeviceID is:%d",NodePdm_Test.u16DeviceID);
    user_zigbeePDM_log("Node is u16ShortAddress is:%4x",NodePdm_Test.u16ShortAddress);
    user_zigbeePDM_log("Node is u64IEEEAddressHigh is:%8x",NodePdm_Test.u32IEEEAddressH);
    user_zigbeePDM_log("Node is u64IEEEAddressLow is:%8x",NodePdm_Test.u32IEEEAddressL);


#endif
#if 0
    //Output
    for(i = 0; i<5; i++)
    {
        printf("0x%x ",read_test[i]);
    }
    printf("\r\n");
#endif
    //MicoFlashWrite( MICO_PARTITION_OTA_TEMP, &context->offset, (uint8_t *)inData, inLen);

    //MicoFlashRead(MICO_PARTITION_OTA_TEMP, &flashaddr, (uint8_t *)md5_recv, 16);

    //err = MicoFlashDisableSecurity( MICO_PARTITION_OTA_TEMP, 0x0, ota_partition_info->partition_length );

    //eSL_AddListener(E_SL_MSG_PDM_AVAILABLE_REQUEST,         PDM_HandleAvailableRequest,     NULL);
    //eSL_AddListener(E_SL_MSG_PDM_LOAD_RECORD_REQUEST,       PDM_HandleLoadRequest,          NULL);
    //eSL_AddListener(E_SL_MSG_PDM_SAVE_RECORD_REQUEST,       PDM_HandleSaveRequest,          NULL);
    //eSL_AddListener(E_SL_MSG_PDM_DELETE_ALL_RECORDS_REQUEST,PDM_HandleDeleteAllRequest,     NULL);


    //mico_rtos_unlock_mutex(&sLock);
    return E_ZCB_OK;
exit:
    //mico_rtos_unlock_mutex(&sLock);
    return E_ZCB_ERROR;
}



/****************************************************************************
* Function	: vPDM_Test
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
void vPDM_Test()
{
#if 0
    //uint8_t i ;
#pragma pack(1)
    tsZCB_NodePDM NodePdm_Test;
#pragma pack()



    NodePdm_Test.u8NodeStatus  = 0xFF;
    NodePdm_Test.u16DeviceID = 0x1234;
    NodePdm_Test.u16ShortAddress = 0x1234;
    NodePdm_Test.u32IEEEAddressH = 0x11223344;
    NodePdm_Test.u32IEEEAddressL = 0x55667788;
    NodePdm_Test.u8MacCapability = 0x8e;
    NodePdm_Test.u16CustomData = 0x0001;

    for(uint8_t i = 0; i<16; i++)
    {
        NodePdm_Test.u16ShortAddress++;
        NodePdm_Test.u16CustomData++;
        ePDM_WriteOneNode(&NodePdm_Test,i);
    }

    for(uint8_t i =0; i<16; i++)
    {
        memset(&NodePdm_Test,0x0,ZIGBEE_NODE_LENGTH);
        ePDM_ReadOneNode(&NodePdm_Test,i);
        user_zigbeePDM_log("Node u8NodeStatus :%8x",NodePdm_Test.u8NodeStatus);
        user_zigbeePDM_log("Node DeviceID :%d",NodePdm_Test.u16DeviceID);
        user_zigbeePDM_log("Node u16ShortAddress :%4x",NodePdm_Test.u16ShortAddress);
        user_zigbeePDM_log("Node u64IEEEAddressHigh :%8x",NodePdm_Test.u32IEEEAddressH);
        user_zigbeePDM_log("Node u64IEEEAddressLow :%8x",NodePdm_Test.u32IEEEAddressL);
        user_zigbeePDM_log("Node u16CustomData :%x",NodePdm_Test.u16CustomData);
        user_zigbeePDM_log("Node u8MacCapability :%8x\r\n",NodePdm_Test.u8MacCapability);
    }

#endif


#if 0

    tsZCB_Node test_node;

    test_node.pasEndpoints = NULL;
    test_node.pau16Groups = NULL;
    test_node.psNext = NULL;
    test_node.sLock = NULL;
    test_node.u16DeviceID = 0x5555;
    test_node.u16ShortAddress = 0x1234;
    test_node.u32NumEndpoints = 5;
    test_node.u32NumGroups = 5;
    test_node.u64IEEEAddress = 0x01;

    user_zigbeePDM_log("PDM test");

    for(uint8_t i = 1; i<16; i++)
    {
        test_node.u64IEEEAddress = i;
        if (ePDM_SaveOneNode(&test_node,0xFF) != E_ZCB_OK)
        {
            user_zigbeePDM_log("Save Node Err");
        }
    }



    for(uint8_t i = 1; i<16; i += 2)
    {
        test_node.u64IEEEAddress = i;		//ffff0001  删过之后 ffffaaaa
        ePDM_DelOneNode(&test_node);
    }

    for(uint8_t i = 0; i<16; i++)
    {
        if(ePDM_ReadOneNode(&NodePdm_Test,i) != E_ZCB_OK)
        {
            user_zigbeePDM_log("read node err");
        }
        else
        {
            user_zigbeePDM_log("Node u32NodeStatus :%8x",NodePdm_Test.u32NodeStatus);
            user_zigbeePDM_log("Node DeviceID :%d",NodePdm_Test.u16DeviceID);
            user_zigbeePDM_log("Node u16ShortAddress :%4x",NodePdm_Test.u16ShortAddress);
            user_zigbeePDM_log("Node u64IEEEAddressHigh :%8x",NodePdm_Test.u32IEEEAddressH);
            user_zigbeePDM_log("Node u64IEEEAddressLow :%8x\r\n",NodePdm_Test.u32IEEEAddressL);
        }
    }

#endif


#if 0
    for(i = 0x00; i<5; i++)
    {
        user_zigbeePDM_log("save the %d node",i);
        test_node.u64IEEEAddress = i;
        if (ePDM_SaveOneNode(&test_node,0xFF) != E_ZCB_OK)
        {
            user_zigbeePDM_log("Save Node Err");
        }
        if(ePDM_ReadOneNode(&NodePdm_Test,0) != E_ZCB_OK)
        {
            user_zigbeePDM_log("read node err");
        }
        else
        {
            user_zigbeePDM_log("Node u32NodeStatus :%8x",NodePdm_Test.u32NodeStatus);
            user_zigbeePDM_log("Node DeviceID :%d",NodePdm_Test.u16DeviceID);
            user_zigbeePDM_log("Node u16ShortAddress :%4x",NodePdm_Test.u16ShortAddress);
            user_zigbeePDM_log("Node u64IEEEAddressHigh :%8x",NodePdm_Test.u32IEEEAddressH);
            user_zigbeePDM_log("Node u64IEEEAddressLow :%8x",NodePdm_Test.u32IEEEAddressL);
        }
        if(ePDM_ReadOneNode(&NodePdm_Test,i) != E_ZCB_OK)
        {
            user_zigbeePDM_log("read node err");
        }
        else
        {

            user_zigbeePDM_log("Node u32NodeStatus :%8x",NodePdm_Test.u32NodeStatus);
            user_zigbeePDM_log("Node DeviceID :%d",NodePdm_Test.u16DeviceID);
            user_zigbeePDM_log("Node u16ShortAddress :%4x",NodePdm_Test.u16ShortAddress);
            user_zigbeePDM_log("Node u64IEEEAddressHigh :%8x",NodePdm_Test.u32IEEEAddressH);
            user_zigbeePDM_log("Node u64IEEEAddressLow :%8x",NodePdm_Test.u32IEEEAddressL);
        }
    }
#endif

#if 0
    for(i = 0; i<5; i++)
    {
        user_zigbeePDM_log("del the %d node",i);
        test_node.u64IEEEAddress = i;
        ePDM_DelOneNode(&test_node);

        if(ePDM_ReadOneNode(&NodePdm_Test,0) != E_ZCB_OK)
        {
            user_zigbeePDM_log("read node err");
        }
        else
        {
            user_zigbeePDM_log("Node u32NodeStatus :%8x",NodePdm_Test.u32NodeStatus);
            user_zigbeePDM_log("Node DeviceID :%d",NodePdm_Test.u16DeviceID);
            user_zigbeePDM_log("Node u16ShortAddress :%4x",NodePdm_Test.u16ShortAddress);
            user_zigbeePDM_log("Node u64IEEEAddressHigh :%8x",NodePdm_Test.u32IEEEAddressH);
            user_zigbeePDM_log("Node u64IEEEAddressLow :%8x",NodePdm_Test.u32IEEEAddressL);
        }

        if(ePDM_ReadOneNode(&NodePdm_Test,i) != E_ZCB_OK)
        {
            user_zigbeePDM_log("read node err");
        }
        else
        {
            user_zigbeePDM_log("Node u32NodeStatus :%8x",NodePdm_Test.u32NodeStatus);
            user_zigbeePDM_log("Node DeviceID :%d",NodePdm_Test.u16DeviceID);
            user_zigbeePDM_log("Node u16ShortAddress :%4x",NodePdm_Test.u16ShortAddress);
            user_zigbeePDM_log("Node u64IEEEAddressHigh :%8x",NodePdm_Test.u32IEEEAddressH);
            user_zigbeePDM_log("Node u64IEEEAddressLow :%8x",NodePdm_Test.u32IEEEAddressL);
        }
    }

#endif



}



teZcbStatus ePDM_Destory()
{
    //teZcbStatus err;
    mico_logic_partition_t *zigbeePDM_partition_info;

    mico_rtos_lock_mutex(&sLock);
    zigbeePDM_partition_info = MicoFlashGetInfo(MICO_PARTITION_ZIGBEEPDM_TEMP);
    MicoFlashErase( MICO_PARTITION_ZIGBEEPDM_TEMP, 0x0, zigbeePDM_partition_info->partition_length);

    mico_rtos_unlock_mutex(&sLock);
    mico_rtos_deinit_mutex(&sLock);

    return E_ZCB_OK;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
* Function	: PDM_HandleAvailableRequest
* Description	: 处理 available 请求
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
#if 0
static void PDM_HandleAvailableRequest(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    user_zigbeePDM_log("Host PDM availability request\n");
    if (eSL_SendMessage(E_SL_MSG_PDM_AVAILABLE_RESPONSE, 0, NULL, NULL) != E_SL_OK)
    {
        user_zigbeePDM_log("Error sending message\n");
    }
}
#endif

#if 0
/****************************************************************************
* Function	: PDM_HandleLoadRequest
* Description	: 处理 load 请求
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
static void PDM_HandleLoadRequest(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    //    sqlite3_stmt *psStatement;
    //    char *pcSQL;
    //    int iError = 1;
    //    int iSentRecords = 0;
    //
    //	  #pragma pack(1)
    //    struct _tPDMLoadRequest
    //    {
    //        uint16_t    u16RecordID;
    //    }*psPDMLoadRecordRequest = (struct _tPDMLoadRequest *)pvMessage;
    //    #pragma pack()
    //#define PDM_BLOCK_SIZE 128
    //	  #pragma pack(1)
    //    struct _tPDMLoadResponse
    //    {
    //        uint8_t     u8Status;
    //        uint16_t    u16RecordID;
    //        uint32_t    u32TotalSize;
    //        uint32_t    u32NumBlocks;
    //        uint32_t    u32CurrentBlock;
    //        uint32_t    u32BlockSize;
    //        uint8_t     au8Data[PDM_BLOCK_SIZE];
    //    }sLoadRecordResponse;
    //    #pragma pack()
    //    memset(&sLoadRecordResponse, 0, sizeof(struct _tPDMLoadResponse));
    //
    //    psPDMLoadRecordRequest->u16RecordID = ntohs(psPDMLoadRecordRequest->u16RecordID);
    //
    //    mico_rtos_lock_mutex(&sLock);
    //
    //    DBG_vPrintf(DBG_PDM, "Load record ID 0x%04X\n", psPDMLoadRecordRequest->u16RecordID);
    //
    //    pcSQL = sqlite3_mprintf("SELECT size,numblocks,block,blocksize,data FROM pdm WHERE id=%d", psPDMLoadRecordRequest->u16RecordID);
    //    DBG_vPrintf(DBG_SQL, "Execute SQL '%s'\n", pcSQL);
    //
    //    if (sqlite3_prepare_v2(pDb, pcSQL, -1, &psStatement, NULL) != SQLITE_OK)
    //    {
    //        DBG_vPrintf(DBG_PDM, "Error preparing query\n");
    //    }
    //    else
    //    {
    //        sLoadRecordResponse.u8Status = 1;
    //        while (sLoadRecordResponse.u8Status)
    //        {
    //            iError = 1;
    //            switch(sqlite3_step(psStatement))
    //            {
    //                case(SQLITE_ROW):
    //                    sLoadRecordResponse.u16RecordID     = htons(psPDMLoadRecordRequest->u16RecordID);
    //                    sLoadRecordResponse.u32TotalSize    = htonl(sqlite3_column_int(psStatement, 0));
    //                    sLoadRecordResponse.u32NumBlocks    = htonl(sqlite3_column_int(psStatement, 1));
    //                    sLoadRecordResponse.u32CurrentBlock = htonl(sqlite3_column_int(psStatement, 2));
    //                    sLoadRecordResponse.u32BlockSize    = htonl(sqlite3_column_int(psStatement, 3));
    //                    memcpy(sLoadRecordResponse.au8Data, sqlite3_column_blob(psStatement, 4), sqlite3_column_bytes(psStatement, 4));
    //
    //                    DBG_vPrintf(DBG_PDM, "Sending record ID 0x%04X (Block %d/%d, size %d/%d)\n",
    //                                psPDMLoadRecordRequest->u16RecordID,
    //                                sqlite3_column_int(psStatement, 2),
    //                                sqlite3_column_int(psStatement, 1),
    //                                sqlite3_column_int(psStatement, 3),
    //                                sqlite3_column_int(psStatement, 0)
    //                    );
    //
    //                    if (eSL_SendMessage(E_SL_MSG_PDM_LOAD_RECORD_RESPONSE,
    //                                        sizeof(struct _tPDMLoadResponse) - PDM_BLOCK_SIZE + sqlite3_column_bytes(psStatement, 4),
    //                                        &sLoadRecordResponse, NULL) != E_SL_OK)
    //                    {
    //                        DBG_vPrintf(DBG_PDM, "Error sending message\n");
    //                    }
    //                    iError = 0;
    //                    iSentRecords++;
    //                    break;
    //                case(SQLITE_DONE):
    //                    if (iSentRecords == 0)
    //                    {
    //                        DBG_vPrintf(DBG_PDM, "Record doesn't exist\n");
    //                    }
    //                    sLoadRecordResponse.u8Status = 0;
    //                    break;
    //                case (SQLITE_ERROR):
    //                    DBG_vPrintf(DBG_PDM, "Error during SQL operation(%s)\n", sqlite3_errmsg(pDb));
    //                    sLoadRecordResponse.u8Status = 0;
    //                    break;
    //                default:
    //                    DBG_vPrintf(DBG_PDM, "Unhandled return from sqlite3\n");
    //                    sLoadRecordResponse.u8Status = 0;
    //            }
    //        }
    //    }
    //
    //    if (iError && sLoadRecordResponse.u8Status == 0)
    //    {
    //        if (eSL_SendMessage(E_SL_MSG_PDM_LOAD_RECORD_RESPONSE, sizeof(uint8_t), &sLoadRecordResponse, NULL) != E_SL_OK)
    //        {
    //            DBG_vPrintf(DBG_PDM, "Error sending message\n");
    //        }
    //    }
    //
    //    DBG_vPrintf(DBG_PDM, "Finished handling request\n");
    //    if (sqlite3_finalize(psStatement) != SQLITE_OK)
    //    {
    //        DBG_vPrintf(DBG_PDM, "Error finalizing statement\n");
    //    }
    //    mico_rtos_unlock_mutex(&sLock);
    //    sqlite3_free(pcSQL);
}
#endif

#if 0
static void PDM_HandleSaveRequest(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    //    sqlite3_stmt *psStatement = NULL;
    //    char *pcSQL = NULL;
    //    enum
    //    {
    //        E_PDM_UNKNOWN,
    //        E_PDM_INSERT,
    //        E_PDM_UPDATE
    //    } eAction = E_PDM_UNKNOWN;
    //    #pragma pack(1)
    //    struct _tPDMSaveRequest
    //    {
    //        uint16_t    u16RecordID;
    //        uint32_t    u32TotalSize;
    //        uint32_t    u32NumBlocks;
    //        uint32_t    u32CurrentBlock;
    //        uint32_t    u32BlockSize;
    //        uint8_t     au8Data[PDM_BLOCK_SIZE];
    //    }*psPDMSaveRecordRequest = (struct _tPDMSaveRequest *)pvMessage;
    //		#pragma pack()
    //    #pragma pack(1)
    //    struct _tPDMSaveResponse
    //    {
    //        uint8_t     u8Status;
    //    }sSaveRecordResponse;
    //    #pragma pack()
    //    // Default error
    //    sSaveRecordResponse.u8Status = 1;
    //
    //    psPDMSaveRecordRequest->u16RecordID     = ntohs(psPDMSaveRecordRequest->u16RecordID);
    //    psPDMSaveRecordRequest->u32TotalSize    = ntohl(psPDMSaveRecordRequest->u32TotalSize);
    //    psPDMSaveRecordRequest->u32NumBlocks    = ntohl(psPDMSaveRecordRequest->u32NumBlocks);
    //    psPDMSaveRecordRequest->u32CurrentBlock = ntohl(psPDMSaveRecordRequest->u32CurrentBlock);
    //    psPDMSaveRecordRequest->u32BlockSize    = ntohl(psPDMSaveRecordRequest->u32BlockSize);
    //
    //    mico_rtos_lock_mutex(&sLock);
    //
    //    DBG_vPrintf(DBG_PDM, "Save record ID 0x%04X (Block %d/%d, size %d/%d)\n",
    //                psPDMSaveRecordRequest->u16RecordID,
    //                psPDMSaveRecordRequest->u32CurrentBlock,
    //                psPDMSaveRecordRequest->u32NumBlocks,
    //                psPDMSaveRecordRequest->u32BlockSize,
    //                psPDMSaveRecordRequest->u32TotalSize);
    //
    //    pcSQL = sqlite3_mprintf("SELECT * FROM pdm WHERE id=%d AND block=%d", psPDMSaveRecordRequest->u16RecordID, psPDMSaveRecordRequest->u32CurrentBlock);
    //    DBG_vPrintf(DBG_SQL, "Execute SQL '%s'\n", pcSQL);
    //
    //    if (sqlite3_prepare_v2(pDb, pcSQL, -1, &psStatement, NULL) != SQLITE_OK)
    //    {
    //        DBG_vPrintf(DBG_PDM, "Error preparing query\n");
    //        goto done;
    //    }
    //
    //    switch(sqlite3_step(psStatement))
    //    {
    //        case(SQLITE_ROW):
    //            // Got row already - so update it.
    //            eAction = E_PDM_UPDATE;
    //            break;
    //        case(SQLITE_DONE):
    //            // Row doesn't exist - insert it.
    //            eAction = E_PDM_INSERT;
    //            break;
    //        case (SQLITE_ERROR):
    //            DBG_vPrintf(DBG_PDM, "Error during SQL operation(%s)\n", sqlite3_errmsg(pDb));
    //            goto done;
    //        default:
    //            DBG_vPrintf(DBG_PDM, "Unhandled return from sqlite3\n");
    //            goto done;
    //    }
    //
    //    if (sqlite3_finalize(psStatement) != SQLITE_OK)
    //    {
    //        DBG_vPrintf(DBG_PDM, "Error finalizing statement\n");
    //        goto done;
    //    }
    //
    //    sqlite3_free(pcSQL);
    //    psStatement = NULL;
    //    pcSQL = NULL;
    //
    //    switch (eAction)
    //    {
    //        case (E_PDM_INSERT):
    //            pcSQL = sqlite3_mprintf("INSERT INTO pdm VALUES (%d,%d,%d,%d,%d,?)",
    //                                    psPDMSaveRecordRequest->u16RecordID,
    //                                    psPDMSaveRecordRequest->u32TotalSize,
    //                                    psPDMSaveRecordRequest->u32NumBlocks,
    //                                    psPDMSaveRecordRequest->u32CurrentBlock,
    //                                    psPDMSaveRecordRequest->u32BlockSize
    //                                    );
    //            DBG_vPrintf(DBG_SQL, "Execute SQL '%s'\n", pcSQL);
    //
    //            if (sqlite3_prepare_v2(pDb, pcSQL, -1, &psStatement, NULL) != SQLITE_OK)
    //            {
    //                DBG_vPrintf(DBG_PDM, "Error preparing query\n");
    //            }
    //            else
    //            {
    //                if (sqlite3_bind_blob(psStatement, 1, psPDMSaveRecordRequest->au8Data, psPDMSaveRecordRequest->u32BlockSize, SQLITE_STATIC) != SQLITE_OK)
    //                {
    //                    DBG_vPrintf(DBG_PDM, "error in bind : %s\n", sqlite3_errmsg(pDb));
    //                    goto done;
    //                }
    //            }
    //            break;
    //
    //        case (E_PDM_UPDATE):
    //            pcSQL = sqlite3_mprintf("UPDATE pdm SET size=%d, numblocks=%d, blocksize=%d,data=? WHERE id=%d AND block=%d",
    //                                    psPDMSaveRecordRequest->u32TotalSize,
    //                                    psPDMSaveRecordRequest->u32NumBlocks,
    //                                    psPDMSaveRecordRequest->u32BlockSize,
    //                                    psPDMSaveRecordRequest->u16RecordID,
    //                                    psPDMSaveRecordRequest->u32CurrentBlock
    //                                    );
    //            DBG_vPrintf(DBG_SQL, "Execute SQL '%s'\n", pcSQL);
    //
    //            if (sqlite3_prepare_v2(pDb, pcSQL, -1, &psStatement, NULL) != SQLITE_OK)
    //            {
    //                DBG_vPrintf(DBG_PDM, "Error preparing query\n");
    //            }
    //            else
    //            {
    //                if (sqlite3_bind_blob(psStatement, 1, psPDMSaveRecordRequest->au8Data, psPDMSaveRecordRequest->u32BlockSize, SQLITE_STATIC) != SQLITE_OK)
    //                {
    //                    DBG_vPrintf(DBG_PDM, "error in bind : %s\n", sqlite3_errmsg(pDb));
    //                    goto done;
    //                }
    //            }
    //            break;
    //
    //        default:
    //            DBG_vPrintf(DBG_PDM, "Unknown action\n");
    //            goto done;
    //    }
    //
    //
    //    switch(sqlite3_step(psStatement))
    //    {
    //        case(SQLITE_DONE):
    //            DBG_vPrintf(DBG_PDM, "Done\n");
    //            sSaveRecordResponse.u8Status = 0;
    //            break;
    //        case (SQLITE_ERROR):
    //            DBG_vPrintf(DBG_PDM, "Error during SQL operation(%s)\n", sqlite3_errmsg(pDb));
    //            goto done;
    //            break;
    //        default:
    //            DBG_vPrintf(DBG_PDM, "Unhandled return from sqlite3\n");
    //            goto done;
    //    }
    //
    //done:
    //    if (eSL_SendMessage(E_SL_MSG_PDM_SAVE_RECORD_RESPONSE, sizeof(struct _tPDMSaveResponse), &sSaveRecordResponse, NULL) != E_SL_OK)
    //    {
    //        DBG_vPrintf(DBG_PDM, "Error sending message\n");
    //    }
    //
    //    DBG_vPrintf(DBG_PDM, "Finished handling request\n");
    //    if (sqlite3_finalize(psStatement) != SQLITE_OK)
    //    {
    //        DBG_vPrintf(DBG_PDM, "Error finalizing after statement\n");
    //    }
    //    mico_rtos_unlock_mutex(&sLock);
    //    sqlite3_free(pcSQL);
}
#endif


/****************************************************************************
* Function	: PDM_HandleDeleteAllRequest
* Description	: 处理 E_SL_MSG_PDM_DELETE_ALL_RECORDS_REQUEST 命令
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
#if 0
static void PDM_HandleDeleteAllRequest(void *pvUser, uint16_t u16Length, void *pvMessage)
{
    //sqlite3_stmt *psStatement;
    char *pcSQL;

    struct _tPDMDeleteAllResponse
    {
        uint8_t     u8Status;
    } sDeleteAllResponse;

    mico_rtos_lock_mutex(&sLock);

    sDeleteAllResponse.u8Status = 0;

    user_zigbeePDM_log("Delete all records\n");

    //pcSQL = sqlite3_mprintf("DELETE FROM pdm");
    user_zigbeePDM_log("Execute SQL '%s'\n", pcSQL);

    //if (sqlite3_prepare_v2(pDb, pcSQL, -1, &psStatement, NULL) != SQLITE_OK)
    {
        user_zigbeePDM_log("Error preparing query\n");
    }
    //else
    {
        //switch(sqlite3_step(psStatement))
        {
            //case(SQLITE_DONE):
            //    sDeleteAllResponse.u8Status = 1;
            //    break;
            //case (SQLITE_ERROR):
            //    user_zigbeePDM_log(DBG_PDM, "Error during SQL operation(%s)\n", sqlite3_errmsg(pDb));
            //    sDeleteAllResponse.u8Status = 0;
            //    break;
            //default:
            //    user_zigbeePDM_log("Unhandled return from sqlite3\n");
            //   sDeleteAllResponse.u8Status = 0;
        }
    }

    if (eSL_SendMessage(E_SL_MSG_PDM_DELETE_ALL_RECORDS_RESPONSE, sizeof(struct _tPDMDeleteAllResponse), &sDeleteAllResponse, NULL) != E_SL_OK)
    {
        user_zigbeePDM_log("Error sending message\n");
    }

    user_zigbeePDM_log("Finished handling request\n");
    //if (sqlite3_finalize(psStatement) != SQLITE_OK)
    {
        user_zigbeePDM_log("Error finalizing statement\n");
    }
    mico_rtos_unlock_mutex(&sLock);
    //sqlite3_free(pcSQL);
}
#endif

#if 0
teZcbStatus ePDM_Save()
{
    OSStatus err;
    teZcbStatus status = E_ZCB_ERROR;
    uint32_t dest_offset = 0;

    dest_offset = 0;
    //Write MICO_PARTITION_ZIGBEEPDM_TEMP							  (uint8_t *)(&NodePdm_Test)
    err = MicoFlashWrite(MICO_PARTITION_ZIGBEEPDM_TEMP, &dest_offset, (uint8_t *)(&NodePdm_Test), ZIGBEE_NODE_LENGTH);
    require_noerr(err, exit);

exit:

    return status;
}
#endif


/****************************************************************************
* Function	: ePDM_SaveOneNode
* Description	: Save One Node in Flash		在 Flash 中保存一个node
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus ePDM_SaveOneNode(tsZCB_Node *psZCBNode,uint16_t u16CustomData)
{
    uint8_t i,j;
    uint16_t u16NodeStatus;
    uint16_t u16alive;
#pragma pack(1)
    tsZCB_NodePDM NodePdm_Temp,NodePdm_Check;
#pragma pack()

    //创建要保存的节点 NodePdm_Temp
    NodePdm_Temp.u8NodeStatus  = 0xFF;
    NodePdm_Temp.u16DeviceID = psZCBNode->u16DeviceID;
    //user_zigbeePDM_log("device id :%d",NodePdm_Temp.u16DeviceID);
    NodePdm_Temp.u16ShortAddress = psZCBNode->u16ShortAddress;
    NodePdm_Temp.u32IEEEAddressH = ((psZCBNode->u64IEEEAddress) & 0xFFFFFFFF00000000) >> 32;
    NodePdm_Temp.u32IEEEAddressL = (psZCBNode->u64IEEEAddress) & 0xFFFFFFFF;
    NodePdm_Temp.u8MacCapability = psZCBNode->u8MacCapability;
    NodePdm_Temp.u16CustomData = u16CustomData;

    //1.读取 Flash 中的 NodeStatus 信息ffff ff
    ePDM_GetNodeStatus(&u16NodeStatus);


    //检测是否已经存在于PDM中
    for(j=0; j<ZIGBEE_NODE_MAX; j++)
    {
        if(((1<<j)& u16NodeStatus)==0)	//j中有node
        {
            ePDM_ReadOneNode(&NodePdm_Check,j);
            if((NodePdm_Check.u32IEEEAddressH == NodePdm_Temp.u32IEEEAddressH) && (NodePdm_Check.u32IEEEAddressL == NodePdm_Temp.u32IEEEAddressL))
            {
                if(NodePdm_Check.u16DeviceID == 0x00)
                {
                    ePDM_WriteOneNode(&NodePdm_Temp,j);
                }
                else
                {
                    user_zigbeePDM_log("no nead to save");
                }


                ePDM_GetNodeAliveStatus(&u16alive);
                u16alive &= (~(1<<j));
                ePDM_UpdateNodeAliveStatus(u16alive);
                return E_ZCB_OK;
            }
            else
            {
            }

        }
        else
        {
        }
    }

    for(i=0; i<ZIGBEE_NODE_MAX; i++)	//通过第一个节点的状态信息,找到没有存节点信息的位置
    {
        if(((1<<i)& u16NodeStatus)!=0)
        {
            //mico_rtos_lock_mutex(&sLock);
            ePDM_WriteOneNode(&NodePdm_Temp,i);
            ePDM_GetNodeAliveStatus(&u16alive);
            u16alive &= (~(1<<j));
            ePDM_UpdateNodeAliveStatus(u16alive);
            break;
        }
    }
    if(i>15)
    {
        user_zigbeePDM_log("No Space to Save");
        goto exit;
    }

    //mico_rtos_unlock_mutex(&sLock);
    return E_ZCB_OK;

exit:
    //mico_rtos_unlock_mutex(&sLock);

    return E_ZCB_ERROR;

}

teZcbStatus ePDM_SaveDeviceAnnounce(void *pvMessage)
{
    teZcbStatus status = E_ZCB_OK;
    tsZCB_Node zcb_node;
    //1.取出节点信息
#pragma pack(1)
    struct _tsDeviceAnnounce
    {
        uint16_t    u16ShortAddress;
        uint64_t    u64IEEEAddress;
        uint8_t     u8MacCapability;
    }*psDeviceAnnounce = (struct _tsDeviceAnnounce *)pvMessage;
#pragma pack()
    psDeviceAnnounce->u16ShortAddress  = ntohs(psDeviceAnnounce->u16ShortAddress);
    psDeviceAnnounce->u64IEEEAddress   = ntoh64(psDeviceAnnounce->u64IEEEAddress);

    memset((void*)(&zcb_node),0x00,sizeof(tsZCB_Node));

    zcb_node.u16ShortAddress = psDeviceAnnounce->u16ShortAddress;
    zcb_node.u64IEEEAddress = psDeviceAnnounce->u64IEEEAddress;
    zcb_node.u8MacCapability = psDeviceAnnounce->u8MacCapability;
	zcb_node.u16DeviceID = 0x00;
	
    //判断节点信息是否已经存在???????????????????????
    //保存节点信息到flash中
    status = ePDM_SaveOneNode(&zcb_node,0xFF);

    return status;
}


teZcbStatus ePDM_FindOneNode(uint16_t u16ShortAddress,uint8_t *pu8index)
{
    uint16_t u16NodeStatus = 0x00,i = 0;
#pragma pack(1)
    tsZCB_NodePDM NodePdm_Temp;
#pragma pack()

    //1.读取 Flash 中的 NodeStatus 信息(存于第一个节点)
    ePDM_GetNodeStatus(&u16NodeStatus);

    //2.遍历所有设备,找到要删除的节点
    for(i=0; i<ZIGBEE_NODE_MAX; i++)
    {
        if(((1<<i)& u16NodeStatus)==0)
        {
            ePDM_ReadOneNode(&NodePdm_Temp,i);
            if(NodePdm_Temp.u16ShortAddress == u16ShortAddress)
            {
				*pu8index = i;
				break;
            }
            else
            {
                continue;
            }
        }
    }

	return E_ZCB_OK;
}


/****************************************************************************
* Function	: ePDM_DelAllNodes
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus ePDM_DelAllNodes()
{
    OSStatus err ;
    user_zigbeePDM_log("Del All PDM Nodes");
    err = MicoFlashErase(MICO_PARTITION_ZIGBEEPDM_TEMP, 0, 256);		//实际erase 4k 数据
    require_noerr(err, exit);

    return E_ZCB_OK;
exit:
    return E_ZCB_ERROR;
}


/****************************************************************************
* Function	: ePDM_DelOneNode
* Description	:	从 Flash 中删除一个 node
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus ePDM_DelOneNode(uint64_t *pu64IEEEAddress)
{
    uint16_t u16NodeStatus = 0x00,i = 0;
#pragma pack(1)
    tsZCB_NodePDM NodePdm_Temp;
#pragma pack()

    //1.读取 Flash 中的 NodeStatus 信息(存于第一个节点)
    ePDM_GetNodeStatus(&u16NodeStatus);
    user_zigbeePDM_log("status:%x",u16NodeStatus);

    //2.遍历所有设备,找到要删除的节点
    for(i=0; i<ZIGBEE_NODE_MAX; i++)
    {
        if(((1<<i)& u16NodeStatus)==0)
        {
            ePDM_ReadOneNode(&NodePdm_Temp,i);
            if(((((*pu64IEEEAddress) & 0xFFFFFFFF00000000) >> 32) == NodePdm_Temp.u32IEEEAddressH) &&  (NodePdm_Temp.u32IEEEAddressL == ((*pu64IEEEAddress) & 0xFFFFFFFF)) )
            {
                //3.假删除,第一个节点对应状态位置1
                user_zigbeePDM_log("del %d node",i);
                ePDM_RemoveOneNode(i);
                break;
            }
            else
            {
                continue;
            }
        }
    }
	
    if(i==ZIGBEE_NODE_MAX)
        user_zigbeePDM_log("not find this node");

    return E_ZCB_OK;
}

/****************************************************************************
* Function	: ePDM_ReadOneNode
* Description	:	读取一个节点信息
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus ePDM_ReadOneNode(tsZCB_NodePDM *pNodePdm_Test,uint8_t index)
{
    OSStatus err;
    if(index >= ZIGBEE_NODE_MAX)
    {
        user_zigbeePDM_log("index out");
        return E_ZCB_ERROR;
    }
    uint32_t dest_offset = ZIGBEE_NODE_ABS_ADDR(index);

    err = MicoFlashRead(MICO_PARTITION_ZIGBEEPDM_TEMP, &dest_offset, (uint8_t *)(pNodePdm_Test), ZIGBEE_NODE_LENGTH);
    require_noerr(err, exit);
    return E_ZCB_OK;
exit:
    return E_ZCB_ERROR;
}


/****************************************************************************
* Function	: ePDM_RemoveOneNode
* Description	:	假删除一个节点
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus ePDM_RemoveOneNode(uint8_t index)
{
    OSStatus err;
    uint16_t u16NodeStatus =0;
    uint32_t dest_offset;
    uint8_t *one_page_temp = malloc(256);

    if(one_page_temp==NULL)
    {
        user_zigbeePDM_log("alloc err");
        return E_ZCB_ERROR;
    }
    //1.读出第一个字的数据
    ePDM_GetNodeStatus(&u16NodeStatus);
    //2.更新对应位
    u16NodeStatus = u16NodeStatus | (1 << index);
    //3.读出数据并修改
    dest_offset = 0;
    err = MicoFlashRead(MICO_PARTITION_ZIGBEEPDM_TEMP, &dest_offset, (uint8_t *)(one_page_temp), 256);
    require_noerr(err, exit);
    memcpy(one_page_temp,(uint8_t *)(&u16NodeStatus),2);

    //4.擦除
    dest_offset = 0;
    err = MicoFlashErase(MICO_PARTITION_ZIGBEEPDM_TEMP, dest_offset, 256);
    require_noerr(err, exit);
    //5.写回修改后的数据
    dest_offset = 0;
    err = MicoFlashWrite(MICO_PARTITION_ZIGBEEPDM_TEMP, &dest_offset, one_page_temp, 256);
    require_noerr(err, exit);
    user_zigbeePDM_log("after remove status %x",u16NodeStatus);
    free(one_page_temp);
    return E_ZCB_OK;

exit:
    free(one_page_temp);
    user_zigbeePDM_log("flash err");
    return E_ZCB_OK;
}


/****************************************************************************
* Function	: ePDM_WriteOneNode
* Description	:写入一个 node 信息到 flash
* Input Para	:	index [0,31]
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus ePDM_WriteOneNode(tsZCB_NodePDM *pNodePdm_Test,uint8_t index)
{
    //[0,15]
    OSStatus err;
    uint8_t page = 0;
    uint8_t *one_page_temp = malloc(256);
    uint16_t nodeStatus = 0;
    uint32_t page_start_addr;
    if(index >=ZIGBEE_NODE_MAX)
        return E_ZCB_ERROR;

    if(NULL == one_page_temp)
    {
        user_zigbeePDM_log("alloc err");
        return E_ZCB_ERROR;
    }
    page = index/16;	//page 1

    page_start_addr = page * 256;	//page start address  256

    uint8_t *memIndex = NULL;

    //1.判断 index,读取对应 page (index/16) 的数据,
    err = MicoFlashRead(MICO_PARTITION_ZIGBEEPDM_TEMP, &page_start_addr, one_page_temp ,256);
    require_noerr(err, exit);

    //2.擦除对应page
    page_start_addr = page * 256;
    err = MicoFlashErase( MICO_PARTITION_ZIGBEEPDM_TEMP, page_start_addr, 256);
    require_noerr(err, exit);

    //3.根据node所在page的索引 (index%16)把要写入的数据存入缓存中
    memIndex = one_page_temp + ZIGBEE_NODE_ABS_ADDR(index%16);
    memcpy(memIndex,(uint8_t*)pNodePdm_Test,ZIGBEE_NODE_LENGTH);
    //4.判断 page,写入缓存数据
    if(index < 16)	//page 0
    {
        memcpy(&nodeStatus,one_page_temp,sizeof(nodeStatus));
        nodeStatus = nodeStatus & (~(1<<index));
        memcpy(one_page_temp,&nodeStatus,sizeof(nodeStatus));

        page_start_addr = page * 256;
        err = MicoFlashWrite(MICO_PARTITION_ZIGBEEPDM_TEMP, &page_start_addr, one_page_temp, 256);
        require_noerr(err, exit);
    }
    else	// page1
    {
        page_start_addr = page * 256;
        err = MicoFlashWrite(MICO_PARTITION_ZIGBEEPDM_TEMP, &page_start_addr, one_page_temp, 256);
        require_noerr(err, exit);

        memset(one_page_temp,0x0,256);
        //读第一 page
        page_start_addr = 0;
        err = MicoFlashRead(MICO_PARTITION_ZIGBEEPDM_TEMP, &page_start_addr, one_page_temp ,256);
        require_noerr(err, exit);

        //擦除第一 page
        //page_start_addr = 0;
        //err = MicoFlashErase( MICO_PARTITION_ZIGBEEPDM_TEMP, page_start_addr, 256);
        //require_noerr(err, exit);

        //修改node status
        memcpy(&nodeStatus,one_page_temp,sizeof(nodeStatus));
        nodeStatus = nodeStatus & (~(1<<index));
        memcpy(one_page_temp,&nodeStatus,sizeof(nodeStatus));

        //回写nodestatus
        page_start_addr = 0;
        err = MicoFlashWrite(MICO_PARTITION_ZIGBEEPDM_TEMP, &page_start_addr, one_page_temp, 256);
        require_noerr(err, exit);

    }

    if(one_page_temp)
        free(one_page_temp);
    return E_ZCB_OK;
exit:
    user_zigbeePDM_log("Write err");
    if(one_page_temp)
        free(one_page_temp);
    return E_ZCB_ERROR;
}





/****************************************************************************
* Function	: ePDM_DisplayAllNode
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus ePDM_DisplayAllNode()
{
#pragma pack(1)
    tsZCB_NodePDM NodePdm_Temp;
#pragma pack()

    int8_t i = 0;
    uint16_t NodeStatus = 0;
    uint16_t NodeAliveStatus = 0;
    ePDM_GetNodeStatus(&NodeStatus);
    ePDM_GetNodeAliveStatus(&NodeAliveStatus);
    user_zigbeePDM_log("Node status :%8x",NodeStatus);
    user_zigbeePDM_log("Node alive status :%8x",NodeAliveStatus);
    for(i=0; i<ZIGBEE_NODE_MAX; i++)
    {
        if(((1<<i)& NodeStatus)==0)
        {
            ePDM_ReadOneNode(&NodePdm_Temp,i);
            user_zigbeePDM_log("the i :%d",i);
            user_zigbeePDM_log("Node u8NodeStatus :%x",NodePdm_Temp.u8NodeStatus);
            user_zigbeePDM_log("Node u16DeviceID :%d",NodePdm_Temp.u16DeviceID);
            user_zigbeePDM_log("Node u16ShortAddress :%4x",NodePdm_Temp.u16ShortAddress);
            user_zigbeePDM_log("Node u32IEEEAddressHigh :%8x",NodePdm_Temp.u32IEEEAddressH);
            user_zigbeePDM_log("Node u32IEEEAddressLow :%8x",NodePdm_Temp.u32IEEEAddressL);
            user_zigbeePDM_log("Node u8MacCapability :%x",NodePdm_Temp.u8MacCapability);
            user_zigbeePDM_log("Node u16CustomData :%x",NodePdm_Temp.u16CustomData);
        }
    }
    return E_ZCB_OK;
}



/****************************************************************************
* Function	: ePDM_UpdateNodeAliveStatus
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus ePDM_UpdateNodeAliveStatus(uint16_t u16alive)
{
    OSStatus err;
    uint32_t dest_offset;
    uint8_t *one_page_temp = malloc(256);

    if(one_page_temp==NULL)
    {
        user_zigbeePDM_log("alloc err");
        return E_ZCB_ERROR;
    }
    //1.读出node alive的数据并修改
    dest_offset = 0;
    err = MicoFlashRead(MICO_PARTITION_ZIGBEEPDM_TEMP, &dest_offset, (uint8_t *)(one_page_temp), 256);
    require_noerr(err, exit);
    memcpy(one_page_temp+2,(uint8_t *)(&u16alive),2);
    //2.擦除
    dest_offset = 0;
    err = MicoFlashErase(MICO_PARTITION_ZIGBEEPDM_TEMP, dest_offset, 256);
    require_noerr(err, exit);
    //2.写回修改后的数据
    dest_offset = 0;
    err = MicoFlashWrite(MICO_PARTITION_ZIGBEEPDM_TEMP, &dest_offset, one_page_temp, 256);
    require_noerr(err, exit);
    user_zigbeePDM_log("after update ,alive status %x",u16alive);
    free(one_page_temp);
    return E_ZCB_OK;
exit:
    free(one_page_temp);
    user_zigbeePDM_log("read err");
    return E_ZCB_ERROR;
}



/****************************************************************************
* Function	: ePDM_GetNodeStatus
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus ePDM_GetNodeStatus(uint16_t *status)
{
    OSStatus err;
    uint32_t dest_offset;
    dest_offset = 0;
    err = MicoFlashRead(MICO_PARTITION_ZIGBEEPDM_TEMP, &dest_offset, (uint8_t *)(status), sizeof(uint16_t));
    require_noerr(err, exit);
    return E_ZCB_OK;
exit:
    user_zigbeePDM_log("read err");
    return E_ZCB_ERROR;
}


/****************************************************************************
* Function	: ePDM_GetNodeAliveStatus
* Description	:
* Input Para	:
* Output Para	:
* Return Value:
****************************************************************************/
teZcbStatus ePDM_GetNodeAliveStatus(uint16_t *status)	//获取每个设备Alive的状态
{
    OSStatus err;
    uint32_t dest_offset;
    dest_offset = 2;
    err = MicoFlashRead(MICO_PARTITION_ZIGBEEPDM_TEMP, &dest_offset, (uint8_t *)(status), sizeof(uint16_t));
    require_noerr(err, exit);
    return E_ZCB_OK;
exit:
    user_zigbeePDM_log("read err");
    return E_ZCB_ERROR;

}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

