#ifndef __SII8334_DRV_H__
#define __SII8334_DRV_H__

#include "mhl_porting.h"
#include "si_8334_regs.h"

/* adjust by user */
#define	MHL_LOGICAL_DEVICE_MAP		(MHL_DEV_LD_AUDIO | MHL_DEV_LD_VIDEO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_GUI )
#define DEVCAP_VAL_DEV_STATE       0
#define DEVCAP_VAL_MHL_VERSION     MHL_VERSION
#define DEVCAP_VAL_DEV_CAT         (MHL_DEV_CAT_SOURCE | MHL_DEV_CATEGORY_POW_BIT)
#define DEVCAP_VAL_ADOPTER_ID_H    (uint8_t)(SILICON_IMAGE_ADOPTER_ID >>   8)
#define DEVCAP_VAL_ADOPTER_ID_L    (uint8_t)(SILICON_IMAGE_ADOPTER_ID & 0xFF)
#define DEVCAP_VAL_VID_LINK_MODE   MHL_DEV_VID_LINK_SUPPRGB444
#define DEVCAP_VAL_AUD_LINK_MODE   MHL_DEV_AUD_LINK_2CH
#define DEVCAP_VAL_VIDEO_TYPE      0
#define DEVCAP_VAL_LOG_DEV_MAP     MHL_LOGICAL_DEVICE_MAP
#define DEVCAP_VAL_BANDWIDTH       0
#define DEVCAP_VAL_FEATURE_FLAG    (MHL_FEATURE_RCP_SUPPORT | MHL_FEATURE_RAP_SUPPORT |MHL_FEATURE_SP_SUPPORT)
#define DEVCAP_VAL_DEVICE_ID_H     (uint8_t)(TRANSCODER_DEVICE_ID>>   8)
#define DEVCAP_VAL_DEVICE_ID_L     (uint8_t)(TRANSCODER_DEVICE_ID& 0xFF)
#define DEVCAP_VAL_SCRATCHPAD_SIZE MHL_SCRATCHPAD_SIZE
#define DEVCAP_VAL_INT_STAT_SIZE   MHL_INT_AND_STATUS_SIZE
#define DEVCAP_VAL_RESERVED        0

#define	T_SRC_VBUS_CBUS_TO_STABLE	(200)	// 100 - 1000 milliseconds. Per MHL 1.0 Specs
#define	T_SRC_WAKE_PULSE_WIDTH_1	(20)	// 20 milliseconds. Per MHL 1.0 Specs
#define	T_SRC_WAKE_PULSE_WIDTH_2	(60)	// 60 milliseconds. Per MHL 1.0 Specs

#define	T_SRC_WAKE_TO_DISCOVER		(200)	// 100 - 1000 milliseconds. Per MHL 1.0 Specs

#define T_SRC_VBUS_CBUS_T0_STABLE 	(500)

// Allow RSEN to stay low this much before reacting.
// Per specs between 100 to 200 ms
#define	T_SRC_RSEN_DEGLITCH			(100)	// (150)

// Wait this much after connection before reacting to RSEN (300-500ms)
// Per specs between 300 to 500 ms
#define	T_SRC_RXSENSE_CHK			(400)

typedef struct {
	uint8_t pollIntervalMs;	// Remember what app set the polling frequency as.

	uint8_t status_0;	// Received status from peer is stored here
	uint8_t status_1;	// Received status from peer is stored here

	uint8_t connectedReady;	// local MHL CONNECTED_RDY register value
	uint8_t linkMode;	// local MHL LINK_MODE register value
	uint8_t mhlHpdStatus;	// keep track of SET_HPD/CLR_HPD
	uint8_t mhlRequestWritePending;

	bool_t mhlConnectionEvent;
	uint8_t mhlConnected;

	uint8_t mhlHpdRSENflags;	// keep track of SET_HPD/CLR_HPD

	// mscMsgArrived == true when a MSC MSG arrives, false when it has been picked up
	bool_t mscMsgArrived;
	uint8_t mscMsgSubCommand;
	uint8_t mscMsgData;

	// Remember FEATURE FLAG of the peer to reject app commands if unsupported
	uint8_t mscFeatureFlag;

	uint8_t cbusReferenceCount;	// keep track of CBUS requests
	// Remember last command, offset that was sent.
	// Mostly for READ_DEVCAP command and other non-MSC_MSG commands
	uint8_t mscLastCommand;
	uint8_t mscLastOffset;
	uint8_t mscLastData;

	// Remember last MSC_MSG command (RCPE particularly)
	uint8_t mscMsgLastCommand;
	uint8_t mscMsgLastData;
	uint8_t mscSaveRcpKeyCode;

#define SCRATCHPAD_SIZE 16
	//  support WRITE_BURST
	uint8_t localScratchPad[SCRATCHPAD_SIZE];
	uint8_t miscFlags;	// such as SCRATCHPAD_BUSY
//  uint8_t     mscData[ 16 ];          // What we got back as message data

	uint8_t ucDevCapCacheIndex;
	uint8_t aucDevCapCache[16];

	uint8_t rapFlags;	// CONTENT ON/OFF
	uint8_t preferredClkMode;
} mhlTx_config_t;

// bits for mhlHpdRSENflags:
typedef enum {
	MHL_HPD = 0x01,
	MHL_RSEN = 0x02,
} MhlHpdRSEN_e;

typedef enum {
	FLAGS_SCRATCHPAD_BUSY = 0x01,
	FLAGS_REQ_WRT_PENDING = 0x02,
	FLAGS_WRITE_BURST_PENDING = 0x04,
	FLAGS_RCP_READY = 0x08,
	FLAGS_HAVE_DEV_CATEGORY = 0x10,
	FLAGS_HAVE_DEV_FEATURE_FLAGS = 0x20,
	FLAGS_SENT_DCAP_RDY = 0x40,
	FLAGS_SENT_PATH_EN = 0x80,
} MiscFlags_e;

typedef enum {
	RAP_CONTENT_ON = 0x01,
} rapFlags_e;

typedef struct {
	uint8_t reqStatus;	// CBUS_IDLE, CBUS_PENDING
	uint8_t retryCount;
	uint8_t command;	// VS_CMD or RCP opcode
	uint8_t offsetData;	// Offset of register on CBUS or RCP data
	uint8_t length;		// Only applicable to write burst. ignored otherwise.
	union {
		uint8_t msgData[16];	// Pointer to message data area.
		unsigned char *pdatabytes;	// pointer for write burst or read many bytes
	} payload_u;
} cbus_req_t;

typedef enum {
	DEVCAP_OFFSET_DEV_STATE = 0x00,
	DEVCAP_OFFSET_MHL_VERSION = 0x01,
	DEVCAP_OFFSET_DEV_CAT = 0x02,
	DEVCAP_OFFSET_ADOPTER_ID_H = 0x03,
	DEVCAP_OFFSET_ADOPTER_ID_L = 0x04,
	DEVCAP_OFFSET_VID_LINK_MODE = 0x05,
	DEVCAP_OFFSET_AUD_LINK_MODE = 0x06,
	DEVCAP_OFFSET_VIDEO_TYPE = 0x07,
	DEVCAP_OFFSET_LOG_DEV_MAP = 0x08,
	DEVCAP_OFFSET_BANDWIDTH = 0x09,
	DEVCAP_OFFSET_FEATURE_FLAG = 0x0A,
	DEVCAP_OFFSET_DEVICE_ID_H = 0x0B,
	DEVCAP_OFFSET_DEVICE_ID_L = 0x0C,
	DEVCAP_OFFSET_SCRATCHPAD_SIZE = 0x0D,
	DEVCAP_OFFSET_INT_STAT_SIZE = 0x0E,
	DEVCAP_OFFSET_RESERVED = 0x0F,
	    // this one must be last
	DEVCAP_SIZE,
} DevCapOffset_e;

// Device Power State
#define MHL_DEV_UNPOWERED		0x00
#define MHL_DEV_INACTIVE		0x01
#define MHL_DEV_QUIET			0x03
#define MHL_DEV_ACTIVE			0x04

// Version that this chip supports
#define	MHL_VER_MAJOR		(0x01 << 4)	// bits 4..7
#define	MHL_VER_MINOR		0x02	// bits 0..3
#define MHL_VERSION						(MHL_VER_MAJOR | MHL_VER_MINOR)

//Device Category
#define	MHL_DEV_CATEGORY_OFFSET				DEVCAP_OFFSET_DEV_CAT
#define	MHL_DEV_CATEGORY_POW_BIT			(BIT4)

#define	MHL_DEV_CAT_SINK					0x01
#define	MHL_DEV_CAT_SOURCE					0x02
#define	MHL_DEV_CAT_DONGLE					0x03
#define	MHL_DEV_CAT_SELF_POWERED_DONGLE		0x13

//Video Link Mode
#define	MHL_DEV_VID_LINK_SUPPRGB444			0x01
#define	MHL_DEV_VID_LINK_SUPPYCBCR444		0x02
#define	MHL_DEV_VID_LINK_SUPPYCBCR422		0x04
#define	MHL_DEV_VID_LINK_SUPP_PPIXEL		0x08
#define	MHL_DEV_VID_LINK_SUPP_ISLANDS		0x10

//Audio Link Mode Support
#define	MHL_DEV_AUD_LINK_2CH				0x01
#define	MHL_DEV_AUD_LINK_8CH				0x02

//Feature Flag in the devcap
#define	MHL_DEV_FEATURE_FLAG_OFFSET			DEVCAP_OFFSET_FEATURE_FLAG
#define	MHL_FEATURE_RCP_SUPPORT				BIT0	// Dongles have freedom to not support RCP
#define	MHL_FEATURE_RAP_SUPPORT				BIT1	// Dongles have freedom to not support RAP
#define	MHL_FEATURE_SP_SUPPORT				BIT2	// Dongles have freedom to not support SCRATCHPAD

/*
#define		MHL_POWER_SUPPLY_CAPACITY		16		// 160 mA current
#define		MHL_POWER_SUPPLY_PROVIDED		16		// 160mA 0r 0 for Wolverine.
#define		MHL_HDCP_STATUS					0		// Bits set dynamically
*/

// VIDEO TYPES
#define		MHL_VT_GRAPHICS					0x00
#define		MHL_VT_PHOTO					0x02
#define		MHL_VT_CINEMA					0x04
#define		MHL_VT_GAMES					0x08
#define		MHL_SUPP_VT						0x80

//Logical Dev Map
#define	MHL_DEV_LD_DISPLAY					(0x01 << 0)
#define	MHL_DEV_LD_VIDEO					(0x01 << 1)
#define	MHL_DEV_LD_AUDIO					(0x01 << 2)
#define	MHL_DEV_LD_MEDIA					(0x01 << 3)
#define	MHL_DEV_LD_TUNER					(0x01 << 4)
#define	MHL_DEV_LD_RECORD					(0x01 << 5)
#define	MHL_DEV_LD_SPEAKER					(0x01 << 6)
#define	MHL_DEV_LD_GUI						(0x01 << 7)

//Bandwidth
#define	MHL_BANDWIDTH_LIMIT					22	// 225 MHz

#define MHL_STATUS_REG_CONNECTED_RDY        0x30
#define MHL_STATUS_REG_LINK_MODE            0x31

#define	MHL_STATUS_DCAP_RDY					BIT0

#define MHL_STATUS_CLK_MODE_MASK            0x07
#define MHL_STATUS_CLK_MODE_PACKED_PIXEL    0x02
#define MHL_STATUS_CLK_MODE_NORMAL          0x03
#define MHL_STATUS_PATH_EN_MASK             0x08
#define MHL_STATUS_PATH_ENABLED             0x08
#define MHL_STATUS_PATH_DISABLED            0x00
#define MHL_STATUS_MUTED_MASK               0x10

#define MHL_RCHANGE_INT                     0x20
#define MHL_DCHANGE_INT                     0x21

#define	MHL_INT_DCAP_CHG					BIT0
#define MHL_INT_DSCR_CHG                    BIT1
#define MHL_INT_REQ_WRT                     BIT2
#define MHL_INT_GRT_WRT                     BIT3

// On INTR_1 the EDID_CHG is located at BIT 0
#define	MHL_INT_EDID_CHG					BIT1

#define		MHL_INT_AND_STATUS_SIZE			0x33	// This contains one nibble each - max offset
#define		MHL_SCRATCHPAD_SIZE				16
#define		MHL_MAX_BUFFER_SIZE				MHL_SCRATCHPAD_SIZE	// manually define highest number

enum {
	MHL_MSC_MSG_RCP = 0x10,	// RCP sub-command
	MHL_MSC_MSG_RCPK = 0x11,	// RCP Acknowledge sub-command
	MHL_MSC_MSG_RCPE = 0x12,	// RCP Error sub-command
	MHL_MSC_MSG_RAP = 0x20,	// Mode Change Warning sub-command
	MHL_MSC_MSG_RAPK = 0x21,	// MCW Acknowledge sub-command
};

#define	RCPE_NO_ERROR				0x00
#define	RCPE_INEEFECTIVE_KEY_CODE	0x01
#define	RCPE_BUSY					0x02
/*
 *
 * MHL spec related defines
 *
 */
enum {
	MHL_ACK = 0x33,		// Command or Data byte acknowledge
	MHL_NACK = 0x34,	// Command or Data byte not acknowledge
	MHL_ABORT = 0x35,	// Transaction abort
	MHL_WRITE_STAT = 0x60 | 0x80,	// 0xE0 - Write one status register strip top bit
	MHL_SET_INT = 0x60,	// Write one interrupt register
	MHL_READ_DEVCAP = 0x61,	// Read one register
	MHL_GET_STATE = 0x62,	// Read CBUS revision level from follower
	MHL_GET_VENDOR_ID = 0x63,	// Read vendor ID value from follower.
	MHL_SET_HPD = 0x64,	// Set Hot Plug Detect in follower
	MHL_CLR_HPD = 0x65,	// Clear Hot Plug Detect in follower
	MHL_SET_CAP_ID = 0x66,	// Set Capture ID for downstream device.
	MHL_GET_CAP_ID = 0x67,	// Get Capture ID from downstream device.
	MHL_MSC_MSG = 0x68,	// VS command to send RCP sub-commands
	MHL_GET_SC1_ERRORCODE = 0x69,	// Get Vendor-Specific command error code.
	MHL_GET_DDC_ERRORCODE = 0x6A,	// Get DDC channel command error code.
	MHL_GET_MSC_ERRORCODE = 0x6B,	// Get MSC command error code.
	MHL_WRITE_BURST = 0x6C,	// Write 1-16 bytes to responder's scratchpad.
	MHL_GET_SC3_ERRORCODE = 0x6D,	// Get channel 3 command error code.
};

#define	MHL_RAP_CONTENT_ON		0x10	// Turn content streaming ON.
#define	MHL_RAP_CONTENT_OFF		0x11	// Turn content streaming OFF.

/*
 *
 * MHL Timings applicable to this driver.
 *
 */

#define	MHL_TX_EVENT_NONE				0x00	/* No event worth reporting.  */
#define	MHL_TX_EVENT_DISCONNECTION		0x01	/* MHL connection has been lost */
#define	MHL_TX_EVENT_CONNECTION			0x02	/* MHL connection has been established */
#define	MHL_TX_EVENT_RCP_READY			0x03	/* MHL connection is ready for RCP */
#define	MHL_TX_EVENT_RCP_RECEIVED		0x04	/* Received an RCP. Key Code in "eventParameter" */
#define	MHL_TX_EVENT_RCPK_RECEIVED		0x05	/* Received an RCPK message */
#define	MHL_TX_EVENT_RCPE_RECEIVED		0x06	/* Received an RCPE message . */
#define	MHL_TX_EVENT_DCAP_CHG			0x07	/* Received DCAP_CHG interrupt */
#define	MHL_TX_EVENT_DSCR_CHG			0x08	/* Received DSCR_CHG interrupt */
#define	MHL_TX_EVENT_POW_BIT_CHG		0x09	/* Peer's power capability has changed */
#define	MHL_TX_EVENT_RGND_MHL			0x0A	/* RGND measurement has determine that the peer is an MHL device */
#define MHL_TX_EVENT_TMDS_ENABLED		0x0B    /* conditions for enabling TMDS have been met and TMDS has been enabled*/
#define MHL_TX_EVENT_TMDS_DISABLED      0x0C    /* TMDS have been disabled */
#define	MHL_TX_RCP_KEY_RELEASE			0xF0    /* Soft interrupt, rcp key release */

#define	MHL_DEV_LD_DISPLAY					(0x01 << 0)
#define	MHL_DEV_LD_VIDEO					(0x01 << 1)
#define	MHL_DEV_LD_AUDIO					(0x01 << 2)
#define	MHL_DEV_LD_MEDIA					(0x01 << 3)
#define	MHL_DEV_LD_TUNER					(0x01 << 4)
#define	MHL_DEV_LD_RECORD					(0x01 << 5)
#define	MHL_DEV_LD_SPEAKER					(0x01 << 6)
#define	MHL_DEV_LD_GUI						(0x01 << 7)

#define SILICON_IMAGE_ADOPTER_ID 322
#define TRANSCODER_DEVICE_ID 0x8334
#define pinWakePulseEn 1
#define pinAllowD3 1

#define	POWER_STATE_D3				3
#define	POWER_STATE_D0_NO_MHL		2
#define	POWER_STATE_D0_MHL			0
#define	POWER_STATE_FIRST_INIT		0xFF

#define	TIMER_FOR_MONITORING				(TIMER_0)

#define TX_HW_RESET_PERIOD		10	// 10 ms.
#define TX_HW_RESET_DELAY			100

typedef enum {
	MHL_TX_EVENT_STATUS_HANDLED = 0,
	MHL_TX_EVENT_STATUS_PASSTHROUGH,
} MhlTxNotifyEventsStatus_e;

typedef enum {
	SCRATCHPAD_FAIL = -4, SCRATCHPAD_BAD_PARAM =
	    -3, SCRATCHPAD_NOT_SUPPORTED = -2, SCRATCHPAD_BUSY =
	    -1, SCRATCHPAD_SUCCESS = 0
} ScratchPadStatus_e;

typedef struct _QueueHeader_t
{
    uint8_t head;   // queue empty condition head == tail
    uint8_t tail,tailSample;
}QueueHeader_t,*PQueueHeader_t;

#define QUEUE_SIZE(x) (sizeof(x.queue)/sizeof(x.queue[0]))
#define MAX_QUEUE_DEPTH(x) (QUEUE_SIZE(x) -1)
#define QUEUE_DEPTH(x) ((x.header.head <= (x.header.tailSample= x.header.tail))?(x.header.tailSample-x.header.head):(QUEUE_SIZE(x)-x.header.head+x.header.tailSample))
#define QUEUE_FULL(x) (QUEUE_DEPTH(x) >= MAX_QUEUE_DEPTH(x))

#define ADVANCE_QUEUE_HEAD(x) { x.header.head = (x.header.head < MAX_QUEUE_DEPTH(x))?(x.header.head+1):0; }
#define ADVANCE_QUEUE_TAIL(x) { x.header.tail = (x.header.tail < MAX_QUEUE_DEPTH(x))?(x.header.tail+1):0; }

#define RETREAT_QUEUE_HEAD(x) { x.header.head = (x.header.head > 0)?(x.header.head-1):MAX_QUEUE_DEPTH(x); }

extern void SiiMhlTxDeviceIsr(void);
extern void SiiMhlTxInitialize(uint8_t pollIntervalMs);
extern int is_mhl_dongle_on(void);
#endif
