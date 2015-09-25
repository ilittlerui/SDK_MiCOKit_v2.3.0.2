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

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

static void PDM_HandleAvailableRequest          (void *pvUser, uint16_t u16Length, void *pvMessage);
static void PDM_HandleLoadRequest               (void *pvUser, uint16_t u16Length, void *pvMessage);
static void PDM_HandleSaveRequest               (void *pvUser, uint16_t u16Length, void *pvMessage);
static void PDM_HandleDeleteAllRequest          (void *pvUser, uint16_t u16Length, void *pvMessage);

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
    //mico_logic_partition_t *p1_info;
    uint8_t read_test[100]= {0};
    uint8_t i = 0;
    uint32_t dest_offset = 0;
    uint8_t data_write[6]= {0x06,0x05,0x04,0x03,0x02,0x01};

    mico_rtos_lock_mutex(&sLock);

#if 0
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


    //MicoFlashWrite(mico_partition_t partition, volatile uint32_t * off_set, uint8_t * inBuffer, uint32_t inBufferLength);

    //Write MICO_PARTITION_ZIGBEEPDM_TEMP
    err = MicoFlashWrite(MICO_PARTITION_ZIGBEEPDM_TEMP, &dest_offset, (uint8_t *)data_write, sizeof(data_write));
    require_noerr(err, exit);
#endif

#if 0
    mico_context -> user_config_data = (void*)data_write;
    mico_context -> user_config_data_size = 10;
    err = mico_system_context_update(mico_context);
    require_noerr(err, exit);
    mico_thread_msleep(1000);
#endif

#if 0
    //Read
    dest_offset = 0;
    err = MicoFlashRead(MICO_PARTITION_ZIGBEEPDM_TEMP, &dest_offset, read_test, 5);
    require_noerr(err, exit);
#endif

#if 0
    err = MicoFlashErase( MICO_PARTITION_PARAMETER_1, 0x0, 60);
    require_noerr(err, exit);
    mico_thread_msleep(10);

    err = MicoFlashWrite( MICO_PARTITION_PARAMETER_1, &dest_offset, "aaaaa", 5);
    require_noerr(err, exit);


    p1_info = MicoFlashGetInfo(MICO_PARTITION_PARAMETER_1);
    err = MicoFlashRead(MICO_PARTITION_PARAMETER_1, &dest_offset, read_test, 60);
    require_noerr(err, exit);
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



    //if (sqlite3_open_v2(pcPDMFile, &pDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL) != SQLITE_OK)
    //{
    //    daemon_log(LOG_ERR, "Error initialising PDM database (%s)", sqlite3_errmsg(pDb));
    //    return E_ZCB_ERROR;
    //}
    //user_zigbeePDM_log("PDM Database opened\n");
    {
        //const char *pcTableDef = "CREATE TABLE IF NOT EXISTS pdm (id INTEGER, size INTEGER, numblocks INTEGER, block INTEGER, blocksize INTEGER, data BLOB, PRIMARY KEY (id,block))";
        //char *pcErr;

        //user_zigbeePDM_log("Execute SQL: '%s'\n", pcTableDef);

        //if (sqlite3_exec(pDb, pcTableDef, NULL, NULL, &pcErr) != SQLITE_OK)
        //{
        //    mico_log("Error creating table (%s)", pcErr);
        //sqlite3_free(pcErr);
        //mico_rtos_unlock_mutex(&sLock);
        //return E_ZCB_ERROR;
        //}
    }
    //user_zigbeePDM_log("PDM Database initialised\n");

    //eSL_AddListener(E_SL_MSG_PDM_AVAILABLE_REQUEST,         PDM_HandleAvailableRequest,     NULL);
    //eSL_AddListener(E_SL_MSG_PDM_LOAD_RECORD_REQUEST,       PDM_HandleLoadRequest,          NULL);
    //eSL_AddListener(E_SL_MSG_PDM_SAVE_RECORD_REQUEST,       PDM_HandleSaveRequest,          NULL);
    //eSL_AddListener(E_SL_MSG_PDM_DELETE_ALL_RECORDS_REQUEST,PDM_HandleDeleteAllRequest,     NULL);


    mico_rtos_unlock_mutex(&sLock);
    return E_ZCB_OK;
exit:
    mico_rtos_unlock_mutex(&sLock);
    return err;
}


teZcbStatus ePDM_Destory()
{
#if 0
    mico_rtos_lock_mutex(&sLock);
    //    if (pDb)
    //    {
    //       sqlite3_close(pDb);
    //        DBG_vPrintf(DBG_PDM, "PDM Database closed\n");
    //    }
    mico_rtos_unlock_mutex(&sLock);
    mico_rtos_deinit_mutex(&sLock);
#endif
    return E_ZCB_OK;
}


/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
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
* Description	: ¥¶¿Ì E_SL_MSG_PDM_DELETE_ALL_RECORDS_REQUEST √¸¡Ó
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

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

