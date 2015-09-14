/**
******************************************************************************
* @file    ZigBeeSerialLink.h
* @author  Sven Yang
* @version V1.0.0
* @date    26-August-2015
* @brief   This header contains the user uart interfaces and ZigBeeSerialLink interfaces
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

#ifndef __ZIGBEESERIALLINK_H_
#define __ZIGBEESERIALLINK_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "MiCO.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define UART_FOR_MCU                        MICO_UART_2

#define PACKED __attribute__((__packed__))



#define DBG_SERIALLINK 0
#define DBG_SERIALLINK_CB 0
#define DBG_SERIALLINK_COMMS 0
#define DBG_SERIALLINK_QUEUE 0

#define SL_START_CHAR   0x01
#define SL_ESC_CHAR     0x02
#define SL_END_CHAR     0x03

#define SL_MAX_MESSAGE_LENGTH 128

#define SL_MAX_MESSAGE_QUEUES 3

#define SL_MAX_CALLBACK_QUEUES 3



/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/** Callback function for a given message type
*  \param pvUser           User supplied pointer to be passed to the callback function
*  \param u16Length        Length of the received message
*  \param pvMessage        Pointer to the message data.
*  \return Nothing
*/
typedef void (*tprSL_MessageCallback)(void *pvUser, uint16_t u16Length, void *pvMessage);


typedef enum
{
    E_SL_OK,
    E_SL_ERROR,
    E_SL_NOMESSAGE,
    E_SL_ERROR_SERIAL,
    E_SL_ERROR_NOMEM,
} teSL_Status;


/** Serial link message types */
typedef enum
{
    /* Common Commands */
    E_SL_MSG_STATUS                         =   0x8000,
    E_SL_MSG_LOG                            =   0x8001,

    E_SL_MSG_DATA_INDICATION                =   0x8002,

    E_SL_MSG_NODE_CLUSTER_LIST              =   0x8003,
    E_SL_MSG_NODE_ATTRIBUTE_LIST            =   0x8004,
    E_SL_MSG_NODE_COMMAND_ID_LIST           =   0x8005,
    E_SL_MSG_RESTART_PROVISIONED            =   0x8006,
    E_SL_MSG_RESTART_FACTORY_NEW            =   0x8007,

    E_SL_MSG_GET_VERSION                    =   0x0010,
    E_SL_MSG_VERSION_LIST                   =   0x8010,

    E_SL_MSG_SET_EXT_PANID                  =   0x0020,
    E_SL_MSG_SET_CHANNELMASK                =   0x0021,
    E_SL_MSG_SET_SECURITY                   =   0x0022,
    E_SL_MSG_SET_DEVICETYPE                 =   0x0023,
    E_SL_MSG_START_NETWORK                  =   0x0024,
    E_SL_MSG_NETWORK_JOINED_FORMED          =   0x8024,
    E_SL_MSG_NETWORK_REMOVE_DEVICE          =   0x0026,
    E_SL_MSG_NETWORK_WHITELIST_ENABLE       =   0x0027,
    E_SL_MSG_AUTHENTICATE_DEVICE_REQUEST    =   0x0028,
    E_SL_MSG_AUTHENTICATE_DEVICE_RESPONSE   =   0x8028,

    E_SL_MSG_RESET                          =   0x0011,
    E_SL_MSG_ERASE_PERSISTENT_DATA          =   0x0012,
    E_SL_MSG_GET_PERMIT_JOIN                =   0x0014,
    E_SL_MSG_GET_PERMIT_JOIN_RESPONSE       =   0x8014,
    E_SL_MSG_BIND                           =   0x0030,
    E_SL_MSG_UNBIND                         =   0x0031,

    E_SL_MSG_NETWORK_ADDRESS_REQUEST        =   0x0040,
    E_SL_MSG_IEEE_ADDRESS_REQUEST           =   0x0041,
    E_SL_MSG_IEEE_ADDRESS_RESPONSE          =   0x8041,
    E_SL_MSG_NODE_DESCRIPTOR_REQUEST        =   0x0042,
    E_SL_MSG_NODE_DESCRIPTOR_RESPONSE       =   0x8042,
    E_SL_MSG_SIMPLE_DESCRIPTOR_REQUEST      =   0x0043,
    E_SL_MSG_SIMPLE_DESCRIPTOR_RESPONSE     =   0x8043,
    E_SL_MSG_POWER_DESCRIPTOR_REQUEST       =   0x0044,
    E_SL_MSG_ACTIVE_ENDPOINT_REQUEST        =   0x0045,
    E_SL_MSG_MATCH_DESCRIPTOR_REQUEST       =   0x0046,
    E_SL_MSG_MATCH_DESCRIPTOR_RESPONSE      =   0x8046,
    E_SL_MSG_MANAGEMENT_LEAVE_REQUEST       =   0x0047,
    E_SL_MSG_LEAVE_CONFIRMATION             =   0x8047,
    E_SL_MSG_LEAVE_INDICATION               =   0x8048,
    E_SL_MSG_PERMIT_JOINING_REQUEST         =   0x0049,
    E_SL_MSG_MANAGEMENT_NETWPRK_UPDATE_REQUEST =0x004A,
    E_SL_MSG_SYSTEM_SERVER_DISCOVERY        =   0x004B,
    E_SL_MSG_COMPLEX_DESCRIPTOR_REQUEST     =   0x004C,
    E_SL_MSG_DEVICE_ANNOUNCE                =   0x004D,
    E_SL_MSG_MANAGEMENT_LQI_REQUEST         =   0x004E,
    E_SL_MSG_MANAGEMENT_LQI_RESPONSE        =   0x804E,

    E_SL_MSG_READ_ATTRIBUTE_REQUEST         =   0x0100,
    E_SL_MSG_READ_ATTRIBUTE_RESPONSE        =   0x8100,
    E_SL_MSG_DEFAULT_RESPONSE               =   0x8101,
    E_SL_MSG_ATTRIBUTE_REPORT               =   0x8102,
    E_SL_MSG_WRITE_ATTRIBUTE_REQUEST        =   0x0103,
    E_SL_MSG_WRITE_ATTRIBUTE_RESPONSE       =   0x8103,

    /* Group Cluster */
    E_SL_MSG_ADD_GROUP_REQUEST              =   0x0060,
    E_SL_MSG_ADD_GROUP_RESPONSE             =   0x8060,
    E_SL_MSG_VIEW_GROUP                     =   0x0061,
    E_SL_MSG_GET_GROUP_MEMBERSHIP_REQUEST   =   0x0062,
    E_SL_MSG_GET_GROUP_MEMBERSHIP_RESPONSE  =   0x8062,
    E_SL_MSG_REMOVE_GROUP_REQUEST           =   0x0063,
    E_SL_MSG_REMOVE_GROUP_RESPONSE          =   0x8063,
    E_SL_MSG_REMOVE_ALL_GROUPS              =   0x0064,
    E_SL_MSG_ADD_GROUP_IF_IDENTIFY          =   0x0065,

    /* Identify Cluster */
    E_SL_MSG_IDENTIFY_SEND                  =   0x0070,
    E_SL_MSG_IDENTIFY_QUERY                 =   0x0071,

    /* Level Cluster */
    E_SL_MSG_MOVE_TO_LEVEL                  =   0x0080,
    E_SL_MSG_MOVE_TO_LEVEL_ONOFF            =   0x0081,
    E_SL_MSG_MOVE_STEP                      =   0x0082,
    E_SL_MSG_MOVE_STOP_MOVE                 =   0x0083,
    E_SL_MSG_MOVE_STOP_ONOFF                =   0x0084,

    /* On/Off Cluster */
    E_SL_MSG_ONOFF                          =   0x0092,

    /* Scenes Cluster */
    E_SL_MSG_VIEW_SCENE                     =   0x00A0,
    E_SL_MSG_ADD_SCENE                      =   0x00A1,
    E_SL_MSG_REMOVE_SCENE                   =   0x00A2,
    E_SL_MSG_REMOVE_SCENE_RESPONSE          =   0x80A2,
    E_SL_MSG_REMOVE_ALL_SCENES              =   0x00A3,
    E_SL_MSG_STORE_SCENE                    =   0x00A4,
    E_SL_MSG_STORE_SCENE_RESPONSE           =   0x80A4,
    E_SL_MSG_RECALL_SCENE                   =   0x00A5,
    E_SL_MSG_SCENE_MEMBERSHIP_REQUEST       =   0x00A6,
    E_SL_MSG_SCENE_MEMBERSHIP_RESPONSE      =   0x80A6,

    /* Colour Cluster */
    E_SL_MSG_MOVE_TO_HUE                    =   0x00B0,
    E_SL_MSG_MOVE_HUE                       =   0x00B1,
    E_SL_MSG_STEP_HUE                       =   0x00B2,
    E_SL_MSG_MOVE_TO_SATURATION             =   0x00B3,
    E_SL_MSG_MOVE_SATURATION                =   0x00B4,
    E_SL_MSG_STEP_SATURATION                =   0x00B5,
    E_SL_MSG_MOVE_TO_HUE_SATURATION         =   0x00B6,
    E_SL_MSG_MOVE_TO_COLOUR                 =   0x00B7,
    E_SL_MSG_MOVE_COLOUR                    =   0x00B8,
    E_SL_MSG_STEP_COLOUR                    =   0x00B9,

    /* ZLL Commands */
    /* Touchlink */
    E_SL_MSG_INITIATE_TOUCHLINK             =   0x00D0,
    E_SL_MSG_TOUCHLINK_STATUS               =   0x00D1,

    /* Identify Cluster */
    E_SL_MSG_IDENTIFY_TRIGGER_EFFECT        =   0x00E0,

    /* On/Off Cluster */
    E_SL_MSG_ONOFF_TIMED                    =   0x0093,
    E_SL_MSG_ONOFF_EFFECTS                  =   0x0094,

    /* Scenes Cluster */
    E_SL_MSG_ADD_ENHANCED_SCENE             =   0x00A7,
    E_SL_MSG_VIEW_ENHANCED_SCENE            =   0x00A8,
    E_SL_MSG_COPY_SCENE                     =   0x00A9,

    /* Colour Cluster */
    E_SL_MSG_ENHANCED_MOVE_TO_HUE           =   0x00BA,
    E_SL_MSG_ENHANCED_MOVE_HUE              =   0x00BB,
    E_SL_MSG_ENHANCED_STEP_HUE              =   0x00BC,
    E_SL_MSG_ENHANCED_MOVE_TO_HUE_SATURATION =  0x00BD,
    E_SL_MSG_COLOUR_LOOP_SET                =   0x00BE,
    E_SL_MSG_STOP_MOVE_STEP                 =   0x00BF,
    E_SL_MSG_MOVE_TO_COLOUR_TEMPERATURE     =   0x00C0,
    E_SL_MSG_MOVE_COLOUR_TEMPERATURE        =   0x00C1,
    E_SL_MSG_STEP_COLOUR_TEMPERATURE        =   0x00C2,

    /* ZHA Commands */
    /* Door Lock Cluster */
    E_SL_MSG_LOCK_UNLOCK_DOOR               =   0x00F0,

    /* Persistant data manager messages */
    E_SL_MSG_PDM_AVAILABLE_REQUEST          =   0x0300,
    E_SL_MSG_PDM_AVAILABLE_RESPONSE         =   0x8300,
    E_SL_MSG_PDM_SAVE_RECORD_REQUEST        =   0x0200,
    E_SL_MSG_PDM_SAVE_RECORD_RESPONSE       =   0x8200,
    E_SL_MSG_PDM_LOAD_RECORD_REQUEST        =   0x0201,
    E_SL_MSG_PDM_LOAD_RECORD_RESPONSE       =   0x8201,
    E_SL_MSG_PDM_DELETE_ALL_RECORDS_REQUEST =   0x0202,
    E_SL_MSG_PDM_DELETE_ALL_RECORDS_RESPONSE=   0x8202,
} teSL_MsgType;


typedef struct
{
    enum
    {
        E_ZB_ADDRESSMODE_BOUND          = 0,
        E_ZB_ADDRESSMODE_GROUP          = 1,
        E_ZB_ADDRESSMODE_SHORT          = 2,
        E_ZB_ADDRESSMODE_IEEE           = 3,
    } eMode;
    union
    {
        uint16_t        u16BoundAddress;
        uint16_t        u16GroupAddress;
        uint16_t        u16ShortAddress;
        uint64_t        u64IEEEAddress;
    } uData;
} tsZB_Address;


/** Status message */
typedef struct
{
    enum
    {
        E_SL_MSG_STATUS_SUCCESS,
        E_SL_MSG_STATUS_INCORRECT_PARAMETERS,
        E_SL_MSG_STATUS_UNHANDLED_COMMAND,
        E_SL_MSG_STATUS_BUSY,
        E_SL_MSG_STATUS_STACK_ALREADY_STARTED,
    } eStatus;
    uint8_t             u8SequenceNo;           /**< Sequence number of outgoing message */
    uint16_t            u16MessageType;         /**< Type of message that this is status to */
    char                acMessage[];            /**< Optional message */
} tsSL_Msg_Status;


/** Default response message */
typedef struct
{
    uint8_t             u8SequenceNo;           /**< Sequence number of outgoing message */
    uint8_t             u8Endpoint;             /**< Source endpoint */
    uint16_t            u16ClusterID;           /**< Source cluster ID */
    uint8_t             u8CommandID;            /**< Source command ID */
    uint8_t             u8Status;               /**< Command status */
} tsSL_Msg_DefaultResponse;


/** Data indication */
typedef struct
{
    uint8_t             u8Status;
    uint16_t            u16ProfileID;
    uint16_t            u16ClusterID;
    uint16_t            u16SourceEndpoint;
    uint16_t            u16DesticationEndpoint;
    tsZB_Address        sSourceAddress;
    tsZB_Address        sDestinationAddress;
    uint8_t             au8Payload[];
} tsSL_Msg_Data_Indication;

//==========================================================================
typedef enum
{
    E_STATE_RX_WAIT_START,
    E_STATE_RX_WAIT_TYPEMSB,
    E_STATE_RX_WAIT_TYPELSB,
    E_STATE_RX_WAIT_LENMSB,
    E_STATE_RX_WAIT_LENLSB,
    E_STATE_RX_WAIT_CRC,
    E_STATE_RX_WAIT_DATA,
} teSL_RxState;


/** Forward definition of callback function entry */
struct _tsSL_CallbackEntry;


/** Linked list structure for a callback function entry */
typedef struct _tsSL_CallbackEntry
{
    uint16_t                u16Type;        /**< Message type for this callback */
    tprSL_MessageCallback   prCallback;     /**< User supplied callback function for this message type */
    void                    *pvUser;        /**< User supplied data for the callback function */
    struct _tsSL_CallbackEntry *psNext;     /**< Pointer to next in linked list */
} tsSL_CallbackEntry;


/** Structure used to contain a message */
typedef struct
{
    uint16_t u16Type;
    uint16_t u16Length;
    uint8_t  au8Message[SL_MAX_MESSAGE_LENGTH];
} tsSL_Message;


/** Structure of data for the serial link */
typedef struct
{
    int     iSerialFd;

#ifndef WIN32
    mico_mutex_t         mutex;
#endif /* WIN32 */

    struct
    {
#ifndef WIN32
        mico_mutex_t         mutex;
#endif /* WIN32 */
        tsSL_CallbackEntry      *psListHead;
    } sCallbacks;

    //tsUtilsQueue sCallbackQueue;
    //tsUtilsThread sCallbackThread;

    // Array of listeners for messages
    // eSL_MessageWait uses this array to wait on incoming messages.
    struct
    {
        uint16_t u16Type;
        uint16_t u16Length;
        uint8_t *pu8Message[100];
#ifndef WIN32
        mico_mutex_t mutex;
        //pthread_cond_t cond_data_available;
#else

#endif /* WIN32 */
    } asReaderMessageQueue[SL_MAX_MESSAGE_QUEUES];


    //tsUtilsThread sSerialReader;
} tsSerialLink;


/** Structure allocated and passed to callback handler thread */
typedef struct
{
    tsSL_Message            sMessage;       /** The received message */
    tprSL_MessageCallback   prCallback;     /**< User supplied callback function for this message type */
    void *                  pvUser;         /**< User supplied data for the callback function */
} tsCallbackThreadData;








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


teSL_Status eSL_Init();

teSL_Status eSL_Destroy(void);


/** Send a command message to the serial device.
*  This also listens for the returned Status message.
*  If one is received, the status for the message is returned, otherwise
*  E_SL_ERROR is returned.
*  \param u16Type          Type of message to send
*  \param pu16Length       Message length
*  \param pvMessage        Message data buffer
*  \param pu8SequenceNo    Pointer to location to receive the outgoing sequence number. May be NULL if no sequence expected.
*/
teSL_Status eSL_SendMessage(uint16_t u16Type, uint16_t u16Length, void *pvMessage, uint8_t *pu8SequenceNo);


/** Wait for a message of the given type to be received from the serial device
*  \param u16Type          Type of message to wait for
*  \param u32WaitTimeout   Maximum time to wait for messages (ms)
*  \param pu16Length       Pointer to location to receive message length
*  \param ppvMessage       Pointer to location to receive a pointer to the message buffer
*                          Once a message buffer has been returned, the calling function
*                          has the responsibility of freeing the buffer,
*  \return E_SL_OK on success
*/
teSL_Status eSL_MessageWait(uint16_t u16Type, uint32_t u32WaitTimeout, uint16_t *pu16Length, void **ppvMessage);

teSL_Status eSL_WriteMessage(uint16_t u16Type, uint16_t u16Length, uint8_t *pu8Data);

teSL_Status eSL_ReadMessage(uint16_t *pu16Type, uint16_t *pu16Length, uint16_t u16MaxLength, uint8_t *pu8Message,uint8_t *inDataBuffer,int recvlen);

void uartRecv_thread(void *inContext);

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
