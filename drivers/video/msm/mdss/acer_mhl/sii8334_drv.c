/*
 * SiI8334 Linux Driver
 *
 * Copyright (C) 2011 Silicon Image Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 *!file     si_drv_mhl_tx.c
 *!brief    Silicon Image implementation of MHL driver.
 */
#include <linux/module.h>
#include <linux/init.h>
#include "mhl_porting.h"
#include "mhl_i2c_access.h"
#include "sii8334_drv.h"

#define	MHL_MAX_RCP_KEY_CODE	(0x7F + 1)	// inclusive

static uint8_t rcpSupportTable[MHL_MAX_RCP_KEY_CODE] = {
	(MHL_DEV_LD_GUI),	// 0x00 = Select
	(MHL_DEV_LD_GUI),	// 0x01 = Up
	(MHL_DEV_LD_GUI),	// 0x02 = Down
	(MHL_DEV_LD_GUI),	// 0x03 = Left
	(MHL_DEV_LD_GUI),	// 0x04 = Right
	0, 0, 0, 0,		// 05-08 Reserved
	(MHL_DEV_LD_GUI),	// 0x09 = Root Menu
	0, 0, 0,		// 0A-0C Reserved
	(MHL_DEV_LD_GUI),	// 0x0D = Select
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0E-1F Reserved
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),	// Numeric keys 0x20-0x29
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA |
	 MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA |
	 MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA |
	 MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA |
	 MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA |
	 MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA |
	 MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA |
	 MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA |
	 MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA |
	 MHL_DEV_LD_TUNER),
	0,			// 0x2A = Dot
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),	// Enter key = 0x2B
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),	// Clear key = 0x2C
	0, 0, 0,		// 2D-2F Reserved
	(MHL_DEV_LD_TUNER),	// 0x30 = Channel Up
	(MHL_DEV_LD_TUNER),	// 0x31 = Channel Dn
	(MHL_DEV_LD_TUNER),	// 0x32 = Previous Channel
	(MHL_DEV_LD_AUDIO),	// 0x33 = Sound Select
	0,			// 0x34 = Input Select
	0,			// 0x35 = Show Information
	0,			// 0x36 = Help
	0,			// 0x37 = Page Up
	0,			// 0x38 = Page Down
	0, 0, 0, 0, 0, 0, 0,	// 0x39-0x3F Reserved
	0,			// 0x40 = Undefined

	(MHL_DEV_LD_SPEAKER),	// 0x41 = Volume Up
	(MHL_DEV_LD_SPEAKER),	// 0x42 = Volume Down
	(MHL_DEV_LD_SPEAKER),	// 0x43 = Mute
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x44 = Play
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_RECORD),	// 0x45 = Stop
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_RECORD),	// 0x46 = Pause
	(MHL_DEV_LD_RECORD),	// 0x47 = Record
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x48 = Rewind
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x49 = Fast Forward
	(MHL_DEV_LD_MEDIA),	// 0x4A = Eject
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA),	// 0x4B = Forward
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA),	// 0x4C = Backward
	0, 0, 0,		// 4D-4F Reserved
	0,			// 0x50 = Angle
	0,			// 0x51 = Subpicture
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 52-5F Reserved
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x60 = Play Function
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x61 = Pause the Play Function
	(MHL_DEV_LD_RECORD),	// 0x62 = Record Function
	(MHL_DEV_LD_RECORD),	// 0x63 = Pause the Record Function
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_RECORD),	// 0x64 = Stop Function

	(MHL_DEV_LD_SPEAKER),	// 0x65 = Mute Function
	(MHL_DEV_LD_SPEAKER),	// 0x66 = Restore Mute Function
	0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0x67-0x6F Undefined or reserved
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0	// 0x70-0x7F Undefined or reserved
};

/*
 * Software power states are a little bit different than the hardware states but
 * a close resemblance exists.
 *
 * D3 matches well with hardware state. In this state we receive RGND interrupts
 * to initiate wake up pulse and device discovery
 *
 * Chip wakes up in D2 mode and interrupts MCU for RGND. Firmware changes the TX
 * into D0 mode and sets its own operation mode as POWER_STATE_D0_NO_MHL because
 * MHL connection has not yet completed.
 *
 * For all practical reasons, firmware knows only two states of hardware - D0 and D3.
 *
 * We move from POWER_STATE_D0_NO_MHL to POWER_STATE_D0_MHL only when MHL connection
 * is established.
 *
 *
 *                             S T A T E     T R A N S I T I O N S
 *
 *
 *                    POWER_STATE_D3                      POWER_STATE_D0_NO_MHL
 *                   /--------------\                        /------------\
 *                  /                \                      /     D0       \
 *                 /                  \                \   /                \
 *                /   DDDDDD  333333   \     RGND       \ /   NN  N  OOO     \
 *                |   D     D     33   |-----------------|    N N N O   O     |
 *                |   D     D  3333    |      IRQ       /|    N  NN  OOO      |
 *                \   D     D      33  /               /  \                  /
 *                 \  DDDDDD  333333  /                    \   CONNECTION   /
 *                  \                /\                     /\             /
 *                   \--------------/  \  TIMEOUT/         /  -------------
 *                         /|\          \-------/---------/        ||
 *                        / | \            500ms\                  ||
 *                          |                     \                ||
 *                          |  RSEN_LOW                            || MHL_EST
 *                           \ (STATUS)                            ||  (IRQ)
 *                            \                                    ||
 *                             \      /------------\              //
 *                              \    /              \            //
 *                               \  /                \          //
 *                                \/                  \ /      //
 *                                 |    CONNECTED     |/======//
 *                                 |                  |\======/
 *                                 \   (OPERATIONAL)  / \
 *                                  \                /
 *                                   \              /
 *                                    \-----------/
 *                                   POWER_STATE_D0_MHL
 */

static bool_t SiiMhlTxSetPathEn(void);
static bool_t SiiMhlTxClrPathEn(void);
static bool_t SiiMhlTxRcpkSend(uint8_t rcpKeyCode);
static void Int1Isr(void);
static int Int4Isr(void);
static void Int5Isr(void);
static void MhlCbusIsr(void);
static void CbusReset(void);
static void SwitchToD0(void);
static void SwitchToD3(void);
static void WriteInitialRegisterValues(void);
static void InitCBusRegs(void);
static void ForceUsbIdSwitchOpen(void);
static void ReleaseUsbIdSwitchOpen(void);
static void MhlTxDrvProcessConnection(void);
static void MhlTxDrvProcessDisconnection(void);
static void SiiMhlTxNotifyDsHpdChange(uint8_t dsHpdStatus);
static void SiiMhlTxNotifyRgndMhl(void);
static void SiiMhlTxNotifyConnection(bool_t mhlConnected);
static void SiiMhlTxGotMhlMscMsg(uint8_t subCommand, uint8_t cmdData);
static void SiiMhlTxMscCommandDone(uint8_t data1);
static void ProcessScdtStatusChange(void);
static void SiiMhlTxGotMhlIntr(uint8_t intr_0, uint8_t intr_1);
static bool_t SiiMhlTxSetDCapRdy(void);
static bool_t SiiMhlTxRapkSend(void);
static void MhlTxResetStates(void);
static bool_t MhlTxSendMscMsg(uint8_t command, uint8_t cmdData);
static void SiiMhlTxRefreshPeerDevCapEntries(void);
static void MhlTxDriveStates(void);
static void SiiMhlTxMscWriteBurstDone(uint8_t data1);
static void SiiMhlTxGotMhlStatus(uint8_t status_0, uint8_t status_1);
static void MhlTxProcessEvents(void);
static bool_t SiiMhlTxRcpeSend(uint8_t rcpeErrorCode);
static bool_t SiiMhlTxReadDevcap(uint8_t offset);

/*
 *To remember the current power state.
 */
static uint8_t fwPowerState = POWER_STATE_FIRST_INIT;

static bool_t tmdsPoweredUp;
static bool_t mhlConnected;
static uint8_t g_chipRevId;

/*
 * To serialize the RCP commands posted to the CBUS engine, this flag
 * is maintained by the function SiiMhlTxDrvSendCbusCommand()
 */
static bool_t mscCmdInProgress;	// false when it is okay to send a new command
/*
 * Preserve Downstream HPD status
 */
static uint8_t dsHpdStatus = 0;

#define	SET_BIT(offset,bitnumber)		SiiRegModify(offset,(1<<bitnumber),(1<<bitnumber))
#define	CLR_BIT(offset,bitnumber)		SiiRegModify(offset,(1<<bitnumber),0x00)
#define	DISABLE_DISCOVERY()			    SiiRegModify(REG_DISC_CTRL1,BIT0,0)
#define	ENABLE_DISCOVERY()				SiiRegModify(REG_DISC_CTRL1,BIT0,BIT0)
#define STROBE_POWER_ON()				SiiRegModify(REG_DISC_CTRL1,BIT1,0)
/*
 *  Look for interrupts on INTR1
 *  6 - MDI_HPD  - downstream hotplug detect (DSHPD)
 */

#define INTR_1_DESIRED_MASK   (BIT6 | BIT5)
#define	UNMASK_INTR_1_INTERRUPTS()		SiiRegWrite(REG_INTR1_MASK, INTR_1_DESIRED_MASK)
#define	MASK_INTR_1_INTERRUPTS()		SiiRegWrite(REG_INTR1_MASK, 0x00)

/*
 *      Look for interrupts on INTR4 (Register 0x74)
 *              7 = RSVD                (reserved)
 *              6 = RGND Rdy    (interested)
 *              5 = MHL DISCONNECT      (interested)
 *              4 = CBUS LKOUT  (interested)
 *              3 = USB EST             (interested)
 *              2 = MHL EST             (interested)
 *              1 = RPWR5V Change       (ignore)
 *              0 = SCDT Change (only if necessary)
 */

#define	INTR_4_DESIRED_MASK				(BIT0 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6)
#define	UNMASK_INTR_4_INTERRUPTS()		SiiRegWrite(REG_INTR4_MASK, INTR_4_DESIRED_MASK)
#define	MASK_INTR_4_INTERRUPTS()		SiiRegWrite(REG_INTR4_MASK, 0x00)

/*
 *Look for interrupts on INTR_5 (Register ??)
 *        4 = FIFO UNDERFLOW      (interested)
 *        3 = FIFO OVERFLOW       (interested)
 */

#define	INTR_5_DESIRED_MASK				(BIT3 | BIT4)
#define	UNMASK_INTR_5_INTERRUPTS()		SiiRegWrite(REG_INTR5_MASK, INTR_5_DESIRED_MASK)
#define	MASK_INTR_5_INTERRUPTS()		SiiRegWrite(REG_INTR5_MASK, 0x00)

/*
 *Look for interrupts on CBUS:CBUS_INTR_STATUS [0xC8:0x08]
 *        7 = RSVD                        (reserved)
 *        6 = MSC_RESP_ABORT      (interested)
 *        5 = MSC_REQ_ABORT       (interested)
 *        4 = MSC_REQ_DONE        (interested)
 *        3 = MSC_MSG_RCVD        (interested)
 *        2 = DDC_ABORT           (interested)
 *        1 = RSVD                        (reserved)
 *        0 = rsvd                        (reserved)
 */
#define	INTR_CBUS1_DESIRED_MASK			(BIT_CBUS_INTRP_DDC_ABRT_EN| BIT_CBUS_INTRP_MSC_MR_MSC_MSG_EN| BIT_CBUS_INTRP_MSC_MT_DONE_EN| BIT_CBUS_INTRP_MSC_MT_ABRT_EN | BIT_CBUS_INTRP_MSC_MR_ABRT_EN)
#define	UNMASK_CBUS1_INTERRUPTS()		SiiRegWrite(REG_CBUS_INTR_ENABLE, INTR_CBUS1_DESIRED_MASK)
#define	MASK_CBUS1_INTERRUPTS()			SiiRegWrite(REG_CBUS_INTR_ENABLE, 0x00)

#define	INTR_CBUS2_DESIRED_MASK			(BIT_CBUS_INTRP_MSC_MR_SET_INT_EN| BIT_CBUS_INTRP_MSC_MR_WRITE_STATE_EN)
#define	UNMASK_CBUS2_INTERRUPTS()			SiiRegWrite(REG_CBUS_MSC_INT2_ENABLE, INTR_CBUS2_DESIRED_MASK)
#define	MASK_CBUS2_INTERRUPTS()			SiiRegWrite(REG_CBUS_MSC_INT2_ENABLE, 0x00)

#define I2C_INACCESSIBLE -1
#define I2C_ACCESSIBLE 1

/*
 * SiiMhlTxDrvAcquireUpstreamHPDControl
 *
 * Acquire the direct control of Upstream HPD.
 */
static void SiiMhlTxDrvAcquireUpstreamHPDControl(void)
{
	// set reg_hpd_out_ovr_en to first control the hpd
	SET_BIT(REG_INT_CTRL, 4);
	TX_DEBUG_PRINT("Drv: Upstream HPD Acquired.\n");
}

/*
 * SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow
 *
 * Acquire the direct control of Upstream HPD.
 */
static void SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow(void)
{
	// set reg_hpd_out_ovr_en to first control the hpd and clear reg_hpd_out_ovr_val
	SiiRegModify(REG_INT_CTRL, BIT5 | BIT4, BIT4);	// Force upstream HPD to 0 when not in MHL mode.
	TX_DEBUG_PRINT("Drv: Upstream HPD Acquired - driven low.\n");
}

/*
 * SiiMhlTxDrvReleaseUpstreamHPDControl
 *
 * Release the direct control of Upstream HPD.
 */
static void SiiMhlTxDrvReleaseUpstreamHPDControl(void)
{
	// Un-force HPD (it was kept low, now propagate to source
	// let HPD float by clearing reg_hpd_out_ovr_en
	CLR_BIT(REG_INT_CTRL, 4);
	TX_DEBUG_PRINT("Drv: Upstream HPD released.\n");
}

static void Int1Isr(void)
{
	uint8_t regIntr1;
	regIntr1 = SiiRegRead(REG_INTR1);
	if (regIntr1) {
		// Clear all interrupts coming from this register.
		SiiRegWrite(REG_INTR1, regIntr1);

		if (BIT6 & regIntr1) {
			uint8_t cbusStatus;
			//
			// Check if a SET_HPD came from the downstream device.
			//
			cbusStatus = SiiRegRead(REG_PRI_XFR_ABORT_REASON);

			// CBUS_HPD status bit
			if (BIT6 & (dsHpdStatus ^ cbusStatus)) {
				uint8_t status = cbusStatus & BIT6;
				TX_DEBUG_PRINT("Drv: Downstream HPD changed to: %02X\n", (int)cbusStatus);

				// Inform upper layer of change in Downstream HPD
				SiiMhlTxNotifyDsHpdChange(status);

				if (status) {
					// this triggers an EDID read if control has not yet been released
					SiiMhlTxDrvReleaseUpstreamHPDControl();
				}
				// Remember
				dsHpdStatus = cbusStatus;
			}
		}
	}
}

static void SiiMhlTxHwReset(uint16_t hwResetPeriod, uint16_t hwResetDelay)
{
	//Do hw reset here
	//AppResetMhlTx(hwResetPeriod, hwResetDelay);
}

/*
 * SiiMhlTxChipInitialize
 *
 * Chip specific initialization.
 * This function is for SiI 8332/8336 Initialization: HW Reset, Interrupt enable.
 */
static bool_t SiiMhlTxChipInitialize(void)
{
	tmdsPoweredUp = false;
	mhlConnected = false;
	mscCmdInProgress = false;	// false when it is okay to send a new command
	dsHpdStatus = 0;
	fwPowerState = POWER_STATE_D0_MHL;
	//SI_OS_DISABLE_DEBUG_CHANNEL(SII_OSAL_DEBUG_SCHEDULER);

	g_chipRevId = SiiRegRead(REG_DEV_REV);

	SiiMhlTxHwReset(TX_HW_RESET_PERIOD, TX_HW_RESET_DELAY);	// call up through the stack to accomplish reset.
	TX_DEBUG_PRINT("Drv: SiiMhlTxChipInitialize: chip rev: %02X chip id: %02X%02x\n",
			(int)g_chipRevId, (int)SiiRegRead(REG_DEV_IDH), (int)SiiRegRead(REG_DEV_IDL));

	// setup device registers. Ensure RGND interrupt would happen.
	WriteInitialRegisterValues();

	//SiiOsMhlTxInterruptEnable();

	// check of PlatformGPIOGet(pinAllowD3) is done inside SwitchToD3
	SwitchToD3();

	return true;
}

/*
 * SiiMhlTxDeviceIsr
 *
 * This function must be called from a master interrupt handler or any polling
 * loop in the host software if during initialization call the parameter
 * interruptDriven was set to true. SiiMhlTxGetEvents will not look at these
 * events assuming firmware is operating in interrupt driven mode. MhlTx component
 * performs a check of all its internal status registers to see if a hardware event
 * such as connection or disconnection has happened or an RCP message has been
 * received from the connected device. Due to the interruptDriven being true,
 * MhlTx code will ensure concurrency by asking the host software and hardware to
 * disable interrupts and restore when completed. Device interrupts are cleared by
 * the MhlTx component before returning back to the caller. Any handling of
 * programmable interrupt controller logic if present in the host will have to
 * be done by the caller after this function returns back.
 *
 * This function has no parameters and returns nothing.
 *
 * This is the master interrupt handler for 9244. It calls sub handlers
 * of interest. Still couple of status would be required to be picked up
 * in the monitoring routine (Sii9244TimerIsr)
 *
 * To react in least amount of time hook up this ISR to processor's
 * interrupt mechanism.
 *
 * Just in case environment does not provide this, set a flag so we
 * call this from our monitor (Sii9244TimerIsr) in periodic fashion.
 *
 * Device Interrupts we would look at
 *              RGND            = to wake up from D3
 *              MHL_EST         = connection establishment
 *              CBUS_LOCKOUT= Service USB switch
 *              CBUS            = responder to peer messages
 *                                        Especially for DCAP etc time based events
 */

void SiiMhlTxDeviceIsr(void)
{
	uint8_t intMStatus, i;	//master int status
	//
	// Look at discovery interrupts if not yet connected.
	//

	i = 0;

	do {
		if (POWER_STATE_D0_MHL != fwPowerState) {
			//
			// Check important RGND, MHL_EST, CBUS_LOCKOUT and SCDT interrupts
			// During D3 we only get RGND but same ISR can work for both states
			//
			if (I2C_INACCESSIBLE == Int4Isr()) {
				return;	// don't do any more I2C traffic until the next interrupt.
			}
		} else if (POWER_STATE_D0_MHL == fwPowerState) {

			if (I2C_INACCESSIBLE == Int4Isr()) {
				return;	// don't do any more I2C traffic until the next interrupt.
			}
			// it is no longer necessary to check if the call to Int4Isr()
			//  put us into D3 mode, since we now return immediately in that case
			Int5Isr();

			// Check for any peer messages for DCAP_CHG etc
			// Dispatch to have the CBUS module working only once connected.
			MhlCbusIsr();
			Int1Isr();
		}

		if (POWER_STATE_D3 != fwPowerState) {
			// Call back into the MHL component to give it a chance to
			// take care of any message processing caused by this interrupt.
			MhlTxProcessEvents();
		}

		intMStatus = SiiRegRead(REG_INTR_STATE);	// read status
		if (0xFF == intMStatus) {
			intMStatus = 0;
			TX_DEBUG_PRINT("\nDrv: EXITING ISR DUE TO intMStatus - 0xFF loop = [%02X] \
				intMStatus = [%02X] \n\n", (int)i, (int)intMStatus);
		}
		i++;

		intMStatus &= 0x01;	//RG mask bit 0
	} while (intMStatus);
}

/*
 * SiiMhlTxDrvTmdsControl
 *
 * Control the TMDS output. MhlTx uses this to support RAP content on and off.
 */
static void SiiMhlTxDrvTmdsControl(bool_t enable)
{
	if (enable) {
		SET_BIT(REG_TMDS_CCTRL, 4);
		TX_DEBUG_PRINT("Drv: TMDS Output Enabled\n");
		SiiMhlTxDrvReleaseUpstreamHPDControl();	// this triggers an EDID read
	} else {
		CLR_BIT(REG_TMDS_CCTRL, 4);
		TX_DEBUG_PRINT("Drv: TMDS Ouput Disabled\n");
	}
}

/*
 * SiiMhlTxDrvNotifyEdidChange
 *
 * MhlTx may need to inform upstream device of an EDID change. This can be
 * achieved by toggling the HDMI HPD signal or by simply calling EDID read
 * function.
 */
static void SiiMhlTxDrvNotifyEdidChange(void)
{
	TX_DEBUG_PRINT("Drv: SiiMhlTxDrvNotifyEdidChange\n");
	//
	// Prepare to toggle HPD to upstream
	//
	SiiMhlTxDrvAcquireUpstreamHPDControl();

	// reg_hpd_out_ovr_val = LOW to force the HPD low
	CLR_BIT(REG_INT_CTRL, 5);

	// wait a bit
	HalTimerWait(110);

	// release HPD back to high by reg_hpd_out_ovr_val = HIGH
	SET_BIT(REG_INT_CTRL, 5);

	// release control to allow transcoder to modulate for CLR_HPD and SET_HPD
	SiiMhlTxDrvReleaseUpstreamHPDControl();
}

/*
 * Function:    SiiMhlTxDrvSendCbusCommand
 *
 * Write the specified Sideband Channel command to the CBUS.
 * Command can be a MSC_MSG command (RCP/RAP/RCPK/RCPE/RAPK), or another command
 * such as READ_DEVCAP, SET_INT, WRITE_STAT, etc.
 *
 * Parameters:
 *              pReq    - Pointer to a cbus_req_t structure containing the
 *                        command to write
 * Returns:     true    - successful write
 *              false   - write failed
 */

static bool_t SiiMhlTxDrvSendCbusCommand(cbus_req_t * pReq)
{
	bool_t success = true;

	uint8_t startbit;

	//
	// If not connected, return with error
	//
	if ((POWER_STATE_D0_MHL != fwPowerState) || (mscCmdInProgress)) {
		TX_DEBUG_PRINT("Error: Drv: fwPowerState: %02X, or CBUS(0x0A):%02X mscCmdInProgress = %d\n", /
				(int)fwPowerState, (int)SiiRegRead(REG_CBUS_STATUS), (int)mscCmdInProgress);

		return false;
	}
	// Now we are getting busy
	mscCmdInProgress = true;

    /****************************************************************************************/
	/* Setup for the command - write appropriate registers and determine the correct        */
	/*                         start bit.                                                   */
    /****************************************************************************************/

	// Set the offset and outgoing data byte right away
	SiiRegWrite(REG_CBUS_PRI_ADDR_CMD, pReq->offsetData);	// set offset
	SiiRegWrite(REG_CBUS_PRI_WR_DATA_1ST, pReq->payload_u.msgData[0]);

	startbit = 0x00;
	switch (pReq->command) {
	case MHL_SET_INT:	// Set one interrupt register = 0x60
		startbit = MSC_START_BIT_WRITE_REG;
		break;

	case MHL_WRITE_STAT:	// Write one status register = 0x60 | 0x80
		startbit = MSC_START_BIT_WRITE_REG;
		break;

	case MHL_READ_DEVCAP:	// Read one device capability register = 0x61
		startbit = MSC_START_BIT_READ_REG;
		break;

	case MHL_GET_STATE:	// 0x62 -
	case MHL_GET_VENDOR_ID:	// 0x63 - for vendor id
	case MHL_SET_HPD:	// 0x64 - Set Hot Plug Detect in follower
	case MHL_CLR_HPD:	// 0x65 - Clear Hot Plug Detect in follower
	case MHL_GET_SC1_ERRORCODE:	// 0x69 - Get channel 1 command error code
	case MHL_GET_DDC_ERRORCODE:	// 0x6A - Get DDC channel command error code.
	case MHL_GET_MSC_ERRORCODE:	// 0x6B - Get MSC command error code.
	case MHL_GET_SC3_ERRORCODE:	// 0x6D - Get channel 3 command error code.
		SiiRegWrite(REG_CBUS_PRI_ADDR_CMD, pReq->command);
		startbit = MSC_START_BIT_MSC_CMD;
		break;

	case MHL_MSC_MSG:
		SiiRegWrite(REG_CBUS_PRI_WR_DATA_2ND,
			    pReq->payload_u.msgData[1]);
		SiiRegWrite(REG_CBUS_PRI_ADDR_CMD, pReq->command);
		startbit = MSC_START_BIT_VS_CMD;
		break;

	case MHL_WRITE_BURST:
		SiiRegWrite(REG_MSC_WRITE_BURST_LEN, pReq->length - 1);

		// Now copy all bytes from array to local scratchpad
		if (NULL == pReq->payload_u.pdatabytes) {
			TX_DEBUG_PRINT("\nDrv: Put pointer to WRITE_BURST data in req.pdatabytes!!!\n\n");
			success = false;
		} else {
			uint8_t *pData = pReq->payload_u.pdatabytes;
			TX_DEBUG_PRINT("\nDrv: Writing data into scratchpad\n\n");
			SiiRegWriteBlock(REG_CBUS_SCRATCHPAD_0, pData, pReq->length);
			startbit = MSC_START_BIT_WRITE_BURST;
		}
		break;

	default:
		success = false;
		break;
	}

    /****************************************************************************************/
	/* Trigger the CBUS command transfer using the determined start bit.                    */
    /****************************************************************************************/

	if (success) {
		SiiRegWrite(REG_CBUS_PRI_START, startbit);
	} else {
		TX_DEBUG_PRINT("\nDrv: SiiMhlTxDrvSendCbusCommand failed\n\n");
	}

	return (success);
}

static bool_t SiiMhlTxDrvCBusBusy(void)
{
	return mscCmdInProgress ? true : false;
}

/*
 * WriteInitialRegisterValues
 *
 */
static void WriteInitialRegisterValues(void)
{
	//TX_DEBUG_PRINT("Drv: WriteInitialRegisterValues\n");

	// Power Up
	SiiRegWrite(REG_DPD, 0x3F);	// Power up CVCC 1.2V core
	SiiRegWrite(REG_TMDS_CLK_EN, 0x01);	// Enable TxPLL Clock
	SiiRegWrite(REG_TMDS_CH_EN, 0x11);	// Enable Tx Clock Path & Equalizer

	SiiRegWrite(REG_MHLTX_CTL1, 0x10);	// TX Source termination ON
	SiiRegWrite(REG_MHLTX_CTL6, 0xBC);	// Enable 1X MHL clock output
	SiiRegWrite(REG_MHLTX_CTL2, 0x3C);	// TX Differential Driver Config
	SiiRegWrite(REG_MHLTX_CTL4, 0xC8);
	SiiRegWrite(REG_MHLTX_CTL7, 0x03);
	SiiRegWrite(REG_MHLTX_CTL8, 0x0A);	// PLL BW Control

	// Analog PLL Control
	SiiRegWrite(REG_TMDS_CCTRL, 0x08);	// Enable Rx PLL clock
	SiiRegWrite(REG_USB_CHARGE_PUMP, 0x8C);
	SiiRegWrite(REG_TMDS_CTRL4, 0x02);

	SiiRegWrite(REG_TMDS0_CCTRL2, 0x00);
	SiiRegModify(REG_DVI_CTRL3, BIT5, 0);
	SiiRegWrite(REG_TMDS_TERMCTRL1, 0x60);

	SiiRegWrite(REG_PLL_CALREFSEL, 0x03);			// PLL Calrefsel
	SiiRegWrite(REG_PLL_VCOCAL, 0x20);			// VCO Cal
	SiiRegWrite(REG_EQ_DATA0, 0xE0);			// Auto EQ
	SiiRegWrite(REG_EQ_DATA1, 0xC0);			// Auto EQ
	SiiRegWrite(REG_EQ_DATA2, 0xA0);			// Auto EQ
	SiiRegWrite(REG_EQ_DATA3, 0x80);			// Auto EQ
	SiiRegWrite(REG_EQ_DATA4, 0x60);			// Auto EQ
	SiiRegWrite(REG_EQ_DATA5, 0x40);			// Auto EQ
	SiiRegWrite(REG_EQ_DATA6, 0x20);			// Auto EQ
	SiiRegWrite(REG_EQ_DATA7, 0x00);			// Auto EQ

	SiiRegWrite(REG_BW_I2C, 0x0A);			// Rx PLL BW ~ 4MHz
	SiiRegWrite(REG_EQ_PLL_CTRL1, 0x06);			// Rx PLL BW value from I2C

	SiiRegWrite(REG_MON_USE_COMP_EN, 0x06);

    // synchronous s/w reset
	SiiRegWrite(REG_ZONE_CTRL_SW_RST, 0x60);			// Manual zone control
	SiiRegWrite(REG_ZONE_CTRL_SW_RST, 0xE0);			// Manual zone control

	SiiRegWrite(REG_MODE_CONTROL, 0x00);			// PLL Mode Value

	SiiRegWrite(REG_SYS_CTRL1, 0x35);	// bring out from power down (script moved this here from above)

	SiiRegWrite(REG_DISC_CTRL2, 0xAD);
	SiiRegWrite(REG_DISC_CTRL5, 0x57);	// 1.8V CBUS VTH 5K pullup for MHL state
	SiiRegWrite(REG_DISC_CTRL6, 0x11);	// RGND & single discovery attempt (RGND blocking)
	SiiRegWrite(REG_DISC_CTRL8, 0x82);	// Ignore VBUS

	if (pinWakePulseEn) {
		SiiRegWrite(REG_DISC_CTRL9, 0x24);	// No OTG, Discovery pulse proceed, Wake pulse not bypassed
	} else {
		SiiRegWrite(REG_DISC_CTRL9, 0x26);	// No OTG, Discovery pulse proceed, Wake pulse bypassed
	}

	// leave bit 3 reg_usb_en at its default value of 1
	SiiRegWrite(REG_DISC_CTRL4, 0x8C);	// Pull-up resistance off for IDLE state and 10K for discovery state.
	SiiRegWrite(REG_DISC_CTRL1, 0x27);	// Enable CBUS discovery
	SiiRegWrite(REG_DISC_CTRL7, 0x20);	// use 1K only setting
	SiiRegWrite(REG_DISC_CTRL3, 0x86);	// MHL CBUS discovery

	// Don't force HPD to 0 during wake-up from D3
	if (fwPowerState != TX_POWER_STATE_D3) {
		SiiRegModify(REG_INT_CTRL, BIT5 | BIT4, BIT4);	// Force HPD to 0 when not in MHL mode.
	}
#ifdef ASSERT_PUSH_PULL
	SiiRegModify(REG_INT_CTRL, BIT6, 0);	// Assert Push/Pull
#endif

	SiiRegWrite(REG_SRST, 0x84);	// Enable Auto soft reset on SCDT = 0

	SiiRegWrite(REG_DCTL, 0x1C);	// HDMI Transcode mode enable

	CbusReset();

	InitCBusRegs();
}

/*
 *InitCBusRegs
 */
static void InitCBusRegs(void)
{
	TX_DEBUG_PRINT("Drv: InitCBusRegs\n");

	SiiRegWrite(REG_CBUS_COMMON_CONFIG, 0xF2);	// Increase DDC translation layer timer
	SiiRegWrite(REG_CBUS_LINK_CONTROL_7, 0x0B);	// Drive High Time. -- changed from 0x0C on 2011-10-03 - 17:00
	SiiRegWrite(REG_CBUS_LINK_CONTROL_8, 0x30);	// Use programmed timing.
	SiiRegWrite(REG_CBUS_DRV_STRENGTH_0, 0x03);	// CBUS Drive Strength

#define DEVCAP_REG(x) (REG_CBUS_DEVICE_CAP_0 | DEVCAP_OFFSET_##x)
	// Setup our devcap
	SiiRegWrite(DEVCAP_REG(DEV_STATE), DEVCAP_VAL_DEV_STATE);
	SiiRegWrite(DEVCAP_REG(MHL_VERSION), DEVCAP_VAL_MHL_VERSION);
	SiiRegWrite(DEVCAP_REG(DEV_CAT), DEVCAP_VAL_DEV_CAT);
	SiiRegWrite(DEVCAP_REG(ADOPTER_ID_H), DEVCAP_VAL_ADOPTER_ID_H);
	SiiRegWrite(DEVCAP_REG(ADOPTER_ID_L), DEVCAP_VAL_ADOPTER_ID_L);
	SiiRegWrite(DEVCAP_REG(VID_LINK_MODE), DEVCAP_VAL_VID_LINK_MODE);
	SiiRegWrite(DEVCAP_REG(AUD_LINK_MODE), DEVCAP_VAL_AUD_LINK_MODE);
	SiiRegWrite(DEVCAP_REG(VIDEO_TYPE), DEVCAP_VAL_VIDEO_TYPE);
	SiiRegWrite(DEVCAP_REG(LOG_DEV_MAP), DEVCAP_VAL_LOG_DEV_MAP);
	SiiRegWrite(DEVCAP_REG(BANDWIDTH), DEVCAP_VAL_BANDWIDTH);
	SiiRegWrite(DEVCAP_REG(FEATURE_FLAG), DEVCAP_VAL_FEATURE_FLAG);
	SiiRegWrite(DEVCAP_REG(DEVICE_ID_H), DEVCAP_VAL_DEVICE_ID_H);
	SiiRegWrite(DEVCAP_REG(DEVICE_ID_L), DEVCAP_VAL_DEVICE_ID_L);
	SiiRegWrite(DEVCAP_REG(SCRATCHPAD_SIZE), DEVCAP_VAL_SCRATCHPAD_SIZE);
	SiiRegWrite(DEVCAP_REG(INT_STAT_SIZE), DEVCAP_VAL_INT_STAT_SIZE);
	SiiRegWrite(DEVCAP_REG(RESERVED), DEVCAP_VAL_RESERVED);

	// Make bits 2,3 (initiator timeout) to 1,1 for register CBUS_LINK_CONTROL_2
	SiiRegModify(REG_CBUS_LINK_CONTROL_2, BIT_CBUS_INITIATOR_TIMEOUT_MASK, BIT_CBUS_INITIATOR_TIMEOUT_MASK);

	// Clear legacy bit on Wolverine TX. and set timeout to 0xF
	SiiRegWrite(REG_MSC_TIMEOUT_LIMIT, 0x0F);

	// Set NMax to 1
	SiiRegWrite(REG_CBUS_LINK_CONTROL_1, 0x01);
	SiiRegModify(REG_CBUS_MSC_COMPATIBILITY_CTRL, BIT_CBUS_CEC_DISABLE, BIT_CBUS_CEC_DISABLE);  // disallow vendor specific commands
	SiiRegModify(TX_PAGE_CBUS | 0x002E, BIT4, BIT4);	// disallow vendor specific commands

}

/*
 * ForceUsbIdSwitchOpen
 */
static void ForceUsbIdSwitchOpen(void)
{
	DISABLE_DISCOVERY();
	SiiRegModify(REG_DISC_CTRL6, BIT6, BIT6);	// Force USB ID switch to open
	SiiRegWrite(REG_DISC_CTRL3, 0x86);
	SiiRegModify(REG_INT_CTRL, BIT5 | BIT4, BIT4);	// Force HPD to 0 when not in Mobile HD mode.
}

/*
 * ReleaseUsbIdSwitchOpen
 */
static void ReleaseUsbIdSwitchOpen(void)
{
	HalTimerWait(50);	// per spec
	SiiRegModify(REG_DISC_CTRL6, BIT6, 0x00);
	ENABLE_DISCOVERY();
}

/*
	SiiMhlTxDrvProcessMhlConnection
		optionally called by the MHL Tx Component after giving the OEM layer the
		first crack at handling the event.
*/
static void SiiMhlTxDrvProcessRgndMhl(void)
{
	SiiRegModify(REG_DISC_CTRL9, BIT0, BIT0);
}

/*
 * ProcessRgnd
 *
 * H/W has detected impedance change and interrupted.
 * We look for appropriate impedance range to call it MHL and enable the
 * hardware MHL discovery logic. If not, disable MHL discovery to allow
 * USB to work appropriately.
 *
 * In current chip a firmware driven slow wake up pulses are sent to the
 * sink to wake that and setup ourselves for full D0 operation.
 */
static void ProcessRgnd(void)
{
	uint8_t rgndImpedance;

	//
	// Impedance detection has completed - process interrupt
	//
	rgndImpedance = SiiRegRead(REG_DISC_STAT2) & 0x03;
	TX_DEBUG_PRINT("Drv: RGND = %02X : \n", (int)rgndImpedance);

	//
	// 00, 01 or 11 means USB.
	// 10 means 1K impedance (MHL)
	//
	// If 1K, then only proceed with wake up pulses
	if (0x02 == rgndImpedance) {

		pr_info("(MHL Device)\n");
		SiiMhlTxNotifyRgndMhl();	// this will call the application and then optionally call
	} else {
		SiiRegModify(REG_DISC_CTRL9, BIT3, BIT3);	// USB Established
		pr_info("(Non-MHL Device)\n");
	}
}

/*
 * SwitchToD0
 * This function performs s/w as well as h/w state transitions.
 *
 * Chip comes up in D2. Firmware must first bring it to full operation
 * mode in D0.
 */
static void SwitchToD0(void)
{
	TX_DEBUG_PRINT("Drv: Switch to D0\n");
	//enable charging
	setting_external_charger();

	//
	// WriteInitialRegisterValues switches the chip to full power mode.
	//
	WriteInitialRegisterValues();

	// Force Power State to ON

	STROBE_POWER_ON();		// Force Power State to ON
	SiiRegModify(TPI_DEVICE_POWER_STATE_CTRL_REG, TX_POWER_STATE_MASK, 0x00);
	fwPowerState = POWER_STATE_D0_NO_MHL;
}

/*
 *SwitchToD3
 *
 *This function performs s/w as well as h/w state transitions.
 */
static void SwitchToD3(void)
{
	//disable charging
	mhl_enable_charging(POWER_SUPPLY_TYPE_BATTERY);

	if (POWER_STATE_D3 != fwPowerState) {
		TX_DEBUG_PRINT("Drv: Switch To D3: pinAllowD3 = %d\n",
				(int)pinAllowD3);

		SiiRegWrite(TX_PAGE_2 | 0x0001, 0x03);
#ifndef	__KERNEL__		//(
		pinM2uVbusCtrlM = 1;
		pinMhlConn = 1;
		pinUsbConn = 0;
#endif //)

		// Force HPD to 0 when not in MHL mode.
		SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow();

		// Change TMDS termination to high impedance on disconnection
		// Bits 1:0 set to 11

		SiiRegWrite(REG_MHLTX_CTL1, 0xD0);

		//
		// GPIO controlled from SiIMon can be utilized to disallow
		// low power mode, thereby allowing SiIMon to read/write register contents.
		// Otherwise SiIMon reads all registers as 0xFF
		//
		if (pinAllowD3) {
			HalTimerWait(50);
			//
			// Change state to D3 by clearing bit 0 of 3D (SW_TPI, Page 1) register
			// ReadModifyWriteIndexedRegister(INDEXED_PAGE_1, 0x3D, BIT0, 0x00);
			//
			CLR_BIT(REG_DPD, 0);

			fwPowerState = POWER_STATE_D3;
		} else {
			fwPowerState = POWER_STATE_D0_NO_MHL;
		}
	}
}

/*
 * Int4Isr
 *
 *
 *      Look for interrupts on INTR4 (Register 0x21)
 *              7 = RSVD                (reserved)
 *              6 = RGND Rdy    (interested)
 *              5 = CBUS Disconnect    (interested)
 *              4 = CBUS LKOUT  (interested)
 *              3 = USB EST             (interested)
 *              2 = MHL EST             (interested)
 *              1 = RPWR5V Change       (ignore)
 *              0 = SCDT Change (interested during D0)
 */
static int Int4Isr(void)
{
	uint8_t int4Status;

	int4Status = SiiRegRead(REG_INTR4);	// read status

	// When I2C is inoperational (D3) and a previous interrupt brought us here, do nothing.
	if (0xFF != int4Status) {
		if (int4Status & BIT0)	// SCDT Status Change
		{
			if (g_chipRevId < 1) {
				ProcessScdtStatusChange();
			}
		}
		// process MHL_EST interrupt
		if (int4Status & BIT2)	// MHL_EST_INT
		{
			MhlTxDrvProcessConnection();
		}
		// process USB_EST interrupt
		else if (int4Status & BIT3) {
			TX_DEBUG_PRINT("Drv: uUSB-A type device detected.\n");
			SiiRegWrite(REG_DISC_STAT2, 0x80);	// Exit D3 via CBUS falling edge
			SwitchToD3();
			return I2C_INACCESSIBLE;
		}

		if (int4Status & BIT5) {
			MhlTxDrvProcessDisconnection();
			return I2C_INACCESSIBLE;
		}

		if ((POWER_STATE_D0_MHL != fwPowerState) && (int4Status & BIT6)) {
			// Switch to full power mode.
			SwitchToD0();

			ProcessRgnd();
		}
		// Can't succeed at these in D3
		if (fwPowerState != POWER_STATE_D3) {
			// CBUS Lockout interrupt?
			if (int4Status & BIT4) {
				TX_DEBUG_PRINT("Drv: CBus Lockout\n");

				ForceUsbIdSwitchOpen();
				ReleaseUsbIdSwitchOpen();
			}
		}
	}
	SiiRegWrite(REG_INTR4, int4Status);	// clear all interrupts
	return I2C_ACCESSIBLE;
}

/*
 * Int5Isr
 *
 *
 *      Look for interrupts on INTR5
 *              7 =
 *              6 =
 *              5 =
 *              4 =
 *              3 =
 *              2 =
 *              1 =
 *              0 =
 */
static void Int5Isr(void)
{
	uint8_t int5Status;

	int5Status = SiiRegRead(REG_INTR5);

#if 0
	if ((int5Status & BIT4) || (int5Status & BIT3))	// FIFO U/O
	{
		TX_DEBUG_PRINT("** int5Status = %02X; Applying MHL FIFO Reset\n", (int)int5Status);
		SiiRegWrite(REG_SRST, 0x94);
		SiiRegWrite(REG_SRST, 0x84);
	}
#endif
	SiiRegWrite(REG_INTR5, int5Status);	// clear all interrupts
}

/*
 *MhlTxDrvProcessConnection
 */
static void MhlTxDrvProcessConnection(void)
{
	TX_DEBUG_PRINT("Drv: MHL Cable Connected. CBUS:0x0A = %02X\n",
			(int)SiiRegRead(REG_CBUS_BUS_STATUS));

	if (POWER_STATE_D0_MHL == fwPowerState) {
		return;
	}
#ifndef	__KERNEL__		//(
	// VBUS control gpio
	pinM2uVbusCtrlM = 0;
	pinMhlConn = 0;
	pinUsbConn = 1;
#endif // )

	//
	// Discovery over-ride: reg_disc_ovride
	//
	SiiRegWrite(REG_MHLTX_CTL1, 0x10);

	fwPowerState = POWER_STATE_D0_MHL;

	//
	// Increase DDC translation layer timer (uint8_t mode)
	// Setting DDC Byte Mode
	//
	SiiRegWrite(REG_CBUS_COMMON_CONFIG, 0xF2);

	// Keep the discovery enabled. Need RGND interrupt
	ENABLE_DISCOVERY();
	// Notify upper layer of cable connection
	SiiMhlTxNotifyConnection(mhlConnected = true);
}

/*
 * MhlTxDrvProcessDisconnection
 */
static void MhlTxDrvProcessDisconnection(void)
{

	TX_DEBUG_PRINT("Drv: MhlTxDrvProcessDisconnection\n");

	// clear all interrupts
	SiiRegWrite(REG_INTR4, SiiRegRead(REG_INTR4));

	SiiRegWrite(REG_MHLTX_CTL1, 0xD0);

	dsHpdStatus &= ~BIT6;	//cable disconnect implies downstream HPD low
	SiiRegWrite(REG_PRI_XFR_ABORT_REASON, dsHpdStatus);
	SiiMhlTxNotifyDsHpdChange(0);

	if (POWER_STATE_D0_MHL == fwPowerState) {
		// Notify upper layer of cable removal
		SiiMhlTxNotifyConnection(false);
	}
	// Now put chip in sleep mode
	SwitchToD3();
}

/*
 * CbusReset
 */
static void CbusReset(void)
{
	//must write 0xff to clear register
	uint8_t enable[4] = {0xff, 0xff, 0xff, 0xff};
	SET_BIT(REG_SRST, 3);
	HalTimerWait(2);
	CLR_BIT(REG_SRST, 3);

	mscCmdInProgress = false;

	// Adjust interrupt mask everytime reset is performed.
	UNMASK_INTR_1_INTERRUPTS();
	UNMASK_INTR_4_INTERRUPTS();
	if (g_chipRevId < 1) {
		UNMASK_INTR_5_INTERRUPTS();
	} else {
		//RG disabled due to auto FIFO reset
		MASK_INTR_5_INTERRUPTS();
	}

	UNMASK_CBUS1_INTERRUPTS();
	UNMASK_CBUS2_INTERRUPTS();

	// Enable WRITE_STAT interrupt for writes to all 4 MSC Status registers.
	SiiRegWriteBlock(REG_CBUS_WRITE_STAT_ENABLE_0, enable, 4);
	// Enable SET_INT interrupt for writes to all 4 MSC Interrupt registers.
	SiiRegWriteBlock(REG_CBUS_SET_INT_ENABLE_0, enable, 4);
}

/*
 *
 * CBusProcessErrors
 *
 */
static uint8_t CBusProcessErrors(uint8_t intStatus)
{
	uint8_t result = 0;
	uint8_t mscAbortReason = 0;
	uint8_t ddcAbortReason = 0;

	/* At this point, we only need to look at the abort interrupts. */

	intStatus &= (BIT_CBUS_MSC_MR_ABRT| BIT_MSC_XFR_ABORT| BIT_DDC_ABORT);

	if (intStatus) {
//      result = ERROR_CBUS_ABORT;              // No Retry will help
		/* If transfer abort or MSC abort, clear the abort reason register. */
		if( intStatus & BIT_DDC_ABORT) {
			result = ddcAbortReason = SiiRegRead(REG_DDC_ABORT_REASON);
			TX_DEBUG_PRINT(("CBUS DDC ABORT happened, reason:: %02X\n", (int)(ddcAbortReason)));
		}

		if ( intStatus & BIT_MSC_XFR_ABORT) {
			result = mscAbortReason = SiiRegRead(REG_PRI_XFR_ABORT_REASON);

			TX_DEBUG_PRINT(("CBUS:: MSC Transfer ABORTED. Clearing 0x0D\n"));
			SiiRegWrite(REG_PRI_XFR_ABORT_REASON, 0xFF);
		}

		if ( intStatus & BIT_CBUS_MSC_MR_ABRT) {
			TX_DEBUG_PRINT(("CBUS:: MSC Peer sent an ABORT. Clearing 0x0E\n"));
			SiiRegWrite(REG_CBUS_PRI_FWR_ABORT_REASON, 0xFF);
		}

		// Now display the abort reason.

		if (mscAbortReason != 0) {
			TX_DEBUG_PRINT("CBUS:: Reason for ABORT is ....0x%02X = ", (int)mscAbortReason);

			if (mscAbortReason & CBUSABORT_BIT_REQ_MAXFAIL) {
				TX_DEBUG_PRINT("Requestor MAXFAIL - retry threshold exceeded\n");
			}
			if (mscAbortReason & CBUSABORT_BIT_PROTOCOL_ERROR) {
				TX_DEBUG_PRINT("Protocol Error\n");
			}
			if (mscAbortReason & CBUSABORT_BIT_REQ_TIMEOUT) {
				TX_DEBUG_PRINT("Requestor translation layer timeout\n");
			}
			if (mscAbortReason & CBUSABORT_BIT_PEER_ABORTED) {
				TX_DEBUG_PRINT("Peer sent an abort\n");
			}
			if (mscAbortReason & CBUSABORT_BIT_UNDEFINED_OPCODE) {
				TX_DEBUG_PRINT("Undefined opcode\n");
			}
		}
	}
	return (result);
}

static void SiiMhlTxDrvGetScratchPad(uint8_t startReg, uint8_t * pData, uint8_t length)
{
	SiiRegReadBlock(REG_CBUS_SCRATCHPAD_0+startReg, pData, length);
}

/*
 SiiMhlTxDrvMscMsgNacked
    returns:
        0 - message was not NAK'ed
        non-zero message was NAK'ed
 */
static int SiiMhlTxDrvMscMsgNacked(void)
{
	if (SiiRegRead(REG_MSC_WRITE_BURST_LEN) & MSC_MT_DONE_NACK_MASK) {
		TX_DEBUG_PRINT(("MSC MSG NAK'ed - retrying...\n\n"));
		return 1;
	}
	return 0;
}


/*
 * MhlCbusIsr
 *
 * Only when MHL connection has been established. This is where we have the
 * first looks on the CBUS incoming commands or returned data bytes for the
 * previous outgoing command.
 *
 * It simply stores the event and allows application to pick up the event
 * and respond at leisure.
 *
 * Look for interrupts on CBUS:CBUS_INTR_STATUS [0xC8:0x08]
 *              7 = RSVD                        (reserved)
 *              6 = MSC_RESP_ABORT      (interested)
 *              5 = MSC_REQ_ABORT       (interested)
 *              4 = MSC_REQ_DONE        (interested)
 *              3 = MSC_MSG_RCVD        (interested)
 *              2 = DDC_ABORT           (interested)
 *              1 = RSVD                        (reserved)
 *              0 = rsvd                        (reserved)
 */
static void MhlCbusIsr(void)
{
	uint8_t cbusInt;
	uint8_t gotData[4];	// Max four status and int registers.

	//
	// Main CBUS interrupts on CBUS_INTR_STATUS
	//
	cbusInt = SiiRegRead(REG_CBUS_INTR_STATUS);

	// When I2C is inoperational (D3) and a previous interrupt brought us here, do nothing.
	if (cbusInt == 0xFF) {
		return;
	}

	if (cbusInt) {
		//
		// Clear all interrupts that were raised even if we did not process
		//
		SiiRegWrite(REG_CBUS_INTR_STATUS, cbusInt);
	}
	// MSC_MSG (RCP/RAP)
	if ((cbusInt & BIT_CBUS_MSC_MR_MSC_MSG)) {
		uint8_t mscMsg[2];
		TX_DEBUG_PRINT("Drv: MSC_MSG Received\n");
		//
		// Two bytes arrive at registers 0x18 and 0x19
		//
		mscMsg[0] = SiiRegRead(REG_CBUS_PRI_VS_CMD);
		mscMsg[1] = SiiRegRead(REG_CBUS_PRI_VS_DATA);

		TX_DEBUG_PRINT("Drv: MSC MSG: %02X %02X\n", (int)mscMsg[0],
				(int)mscMsg[1]);
		SiiMhlTxGotMhlMscMsg(mscMsg[0], mscMsg[1]);
	}
	if (cbusInt & (BIT_MSC_ABORT | BIT_MSC_XFR_ABORT | BIT_DDC_ABORT)) {
		gotData[0] = CBusProcessErrors(cbusInt);
	}
	// MSC_REQ_DONE received.
	if (cbusInt & BIT_CBUS_MSC_MT_DONE) {
		mscCmdInProgress = false;

		// only do this after cBusInt interrupts are cleared above
		SiiMhlTxMscCommandDone(SiiRegRead(REG_CBUS_PRI_RD_DATA_1ST));
	}
	//
	// Now look for interrupts on register 0x1E. CBUS_MSC_INT2
	// 7:4 = Reserved
	//   3 = msc_mr_write_state = We got a WRITE_STAT
	//   2 = msc_mr_set_int. We got a SET_INT
	//   1 = reserved
	//   0 = msc_mr_write_burst. We received WRITE_BURST
	//
	cbusInt = SiiRegRead(REG_CBUS_MSC_INT2_STATUS);
	if (cbusInt) {
		//
		// Clear all interrupts that were raised even if we did not process
		//
		SiiRegWrite(REG_CBUS_MSC_INT2_STATUS, cbusInt);

		TX_DEBUG_PRINT("Drv: Clear CBUS INTR_2: %02X\n",
				(int)cbusInt);
	}

	if (BIT_CBUS_MSC_MR_WRITE_BURST & cbusInt) {
		// WRITE_BURST complete
		SiiMhlTxMscWriteBurstDone(cbusInt);
	}

	if (cbusInt & BIT_CBUS_MSC_MR_SET_INT) {
		uint8_t intr[4];

		TX_DEBUG_PRINT("Drv: MHL INTR Received\n");

	    SiiRegReadBlock(REG_CBUS_SET_INT_0, intr, 4);
	    SiiRegWriteBlock(REG_CBUS_SET_INT_0, intr, 4);

		// We are interested only in first two bytes.
		SiiMhlTxGotMhlIntr(intr[0], intr[1]);
	}

	if (cbusInt & BIT_CBUS_MSC_MR_WRITE_STATE) {
		uint8_t status[4];
		uint8_t clear[4]={0xff, 0xff, 0xff, 0xff};
		// must write 0xFF to clear regardless!

		// don't put debug output here, it just creates confusion.
	    SiiRegReadBlock(REG_CBUS_WRITE_STAT_0, status, 4);
	    SiiRegWriteBlock(REG_CBUS_WRITE_STAT_0, clear, 4);
		SiiMhlTxGotMhlStatus(status[0], status[1]);
	}

}

static void ProcessScdtStatusChange(void)
{
	uint8_t scdtStatus;
	uint8_t mhlFifoStatus;

	scdtStatus = SiiRegRead(REG_TMDS_CSTAT);

	TX_DEBUG_PRINT("Drv: ProcessScdtStatusChange scdtStatus: 0x%02x\n",
			scdtStatus);

	if (scdtStatus & 0x02) {
		mhlFifoStatus = SiiRegRead(REG_INTR5);
		TX_DEBUG_PRINT("MHL FIFO status: 0x%02x\n", mhlFifoStatus);
		if (mhlFifoStatus & 0x0C) {
			SiiRegWrite(REG_INTR5, 0x0C);

			TX_DEBUG_PRINT("** Apply MHL FIFO Reset\n");
			SiiRegWrite(REG_SRST, 0x94);
			SiiRegWrite(REG_SRST, 0x84);
		}
	}
}

/*
	SiMhlTxDrvSetClkMode
	-- Set the hardware this this clock mode.
 */
static void SiMhlTxDrvSetClkMode(uint8_t clkMode)
{
	TX_DEBUG_PRINT("SiMhlTxDrvSetClkMode:0x%02x\n", (int)clkMode);
	// nothing to do here since we only suport MHL_STATUS_CLK_MODE_NORMAL
	// if we supported SUPP_PPIXEL, this would be the place to write the register
}

int is_mhl_dongle_on(void)
{
	return (POWER_STATE_D0_MHL == fwPowerState) ? 1 : 0;
}

#define NUM_CBUS_EVENT_QUEUE_EVENTS 5
typedef struct _CBusQueue_t {
	QueueHeader_t header;
	cbus_req_t queue[NUM_CBUS_EVENT_QUEUE_EVENTS];
} CBusQueue_t, *PCBusQueue_t;

/*
 *Because the Linux driver can be opened multiple times it can't
 *depend on one time structure initialization done by the compiler.
 */
//CBusQueue_t CBusQueue={0,0,{0}};
CBusQueue_t CBusQueue;

cbus_req_t *GetNextCBusTransactionImpl(void)
{
	if (0 == QUEUE_DEPTH(CBusQueue)) {
		return NULL;
	} else {
		cbus_req_t *retVal;
		retVal = &CBusQueue.queue[CBusQueue.header.head];
		ADVANCE_QUEUE_HEAD(CBusQueue)
		    return retVal;
	}
}

#ifdef ENABLE_CBUS_DEBUG_PRINT	//(
cbus_req_t *GetNextCBusTransactionWrapper(char *pszFunction, int iLine)
{
	CBUS_DEBUG_PRINT("MhlTx:%d GetNextCBusTransaction: %s depth: %d  head: %d  tail: %d\n", iLine, pszFunction, (int)QUEUE_DEPTH(CBusQueue), (int)CBusQueue.header.head, (int)CBusQueue.header.tail);
	return GetNextCBusTransactionImpl();
}

#define GetNextCBusTransaction(func) GetNextCBusTransactionWrapper(#func,__LINE__)
#else //)(
#define GetNextCBusTransaction(func) GetNextCBusTransactionImpl()
#endif //)

static bool_t PutNextCBusTransactionImpl(cbus_req_t * pReq)
{
	if (QUEUE_FULL(CBusQueue)) {
		//queue is full
		return false;
	}
	// at least one slot available
	CBusQueue.queue[CBusQueue.header.tail] = *pReq;
	ADVANCE_QUEUE_TAIL(CBusQueue)
	    return true;
}

#ifdef ENABLE_CBUS_DEBUG_PRINT	//(
// use this wrapper to do debugging output for the routine above.
static bool_t PutNextCBusTransactionWrapper(cbus_req_t * pReq, int iLine)
{
	bool_t retVal;

	CBUS_DEBUG_PRINT("MhlTx:%d PutNextCBusTransaction %02X %02X %02X depth:%d head: %d tail:%d\n",
			iLine, (int)pReq->command,
			(int)((MHL_MSC_MSG == pReq->command) ? pReq->payload_u.msgData[0] : pReq->offsetData),
			(int)((MHL_MSC_MSG == pReq->command) ? pReq->payload_u.msgData[1] : pReq->payload_u.msgData[0]),
			(int)QUEUE_DEPTH(CBusQueue), (int)CBusQueue.header.head, (int)CBusQueue.header.tail);
	retVal = PutNextCBusTransactionImpl(pReq);

	if (!retVal) {
		CBUS_DEBUG_PRINT("MhlTx:%d PutNextCBusTransaction queue full, when adding event %02x\n",
				iLine, (int)pReq->command);
	}
	return retVal;
}

#define PutNextCBusTransaction(req) PutNextCBusTransactionWrapper(req,__LINE__)
#else //)(
#define PutNextCBusTransaction(req) PutNextCBusTransactionImpl(req)
#endif //)

static bool_t PutPriorityCBusTransactionImpl(cbus_req_t * pReq)
{
	if (QUEUE_FULL(CBusQueue)) {
		//queue is full
		return false;
	}
	// at least one slot available
	RETREAT_QUEUE_HEAD(CBusQueue)
	    CBusQueue.queue[CBusQueue.header.head] = *pReq;
	return true;
}

#ifdef ENABLE_CBUS_DEBUG_PRINT	//(
static bool_t PutPriorityCBusTransactionWrapper(cbus_req_t * pReq, int iLine)
{
	bool_t retVal;
	CBUS_DEBUG_PRINT("MhlTx:%d: PutPriorityCBusTransaction %02X %02X %02X depth:%d head: %d tail:%d\n",
			iLine,
			(int)pReq->command, (int)((MHL_MSC_MSG == pReq->command) ? pReq->payload_u.msgData[0] : pReq->offsetData),
			(int)((MHL_MSC_MSG == pReq->command) ? pReq->payload_u.msgData[1] : pReq->payload_u.msgData[0]),
			(int)QUEUE_DEPTH(CBusQueue),
			(int)CBusQueue.header.head, (int)CBusQueue.header.tail);
	retVal = PutPriorityCBusTransactionImpl(pReq);
	if (!retVal) {
		CBUS_DEBUG_PRINT("MhlTx:%d: PutPriorityCBusTransaction queue full, when adding event 0x%02X\n",
				iLine, (int)pReq->command);
	}
	return retVal;
}

#define PutPriorityCBusTransaction(pReq) PutPriorityCBusTransactionWrapper(pReq,__LINE__)
#else //)(
#define PutPriorityCBusTransaction(pReq) PutPriorityCBusTransactionImpl(pReq)
#endif //)

#define IncrementCBusReferenceCount(func) {mhlTxConfig.cbusReferenceCount++; CBUS_DEBUG_PRINT("MhlTx:%s cbusReferenceCount:%d\n",#func,(int)mhlTxConfig.cbusReferenceCount); }
#define DecrementCBusReferenceCount(func) {mhlTxConfig.cbusReferenceCount--; CBUS_DEBUG_PRINT("MhlTx:%s cbusReferenceCount:%d\n",#func,(int)mhlTxConfig.cbusReferenceCount); }

#define SetMiscFlag(func,x) { mhlTxConfig.miscFlags |=  (x); TX_DEBUG_PRINT("MhlTx:%s set %s\n",#func,#x); }
#define ClrMiscFlag(func,x) { mhlTxConfig.miscFlags &= ~(x); TX_DEBUG_PRINT("MhlTx:%s clr %s\n",#func,#x); }

/*
 * Store global config info here. This is shared by the driver.
 *
 *
 *
 * structure to hold operating information of MhlTx component
 */

static mhlTx_config_t mhlTxConfig = { 0 };
/*
	QualifyPathEnable
		Support MHL 1.0 sink devices.

	return value
		1 - consider PATH_EN received
		0 - consider PATH_EN NOT received
 */
static uint8_t QualifyPathEnable(void)
{
	uint8_t retVal = 0;	// return fail by default
	if (MHL_STATUS_PATH_ENABLED & mhlTxConfig.status_1) {
		TX_DEBUG_PRINT("\t\t\tMHL_STATUS_PATH_ENABLED\n");
		retVal = 1;
	} else {
		//first check for DEVCAP refresh inactivity
		if (mhlTxConfig.ucDevCapCacheIndex >= sizeof(mhlTxConfig.aucDevCapCache)) {
			if (0x10 ==
				mhlTxConfig.aucDevCapCache[DEVCAP_OFFSET_MHL_VERSION]) {
				if (0x44 == mhlTxConfig.aucDevCapCache[DEVCAP_OFFSET_INT_STAT_SIZE]) {
						retVal = 1;
						TX_DEBUG_PRINT("\t\t\tLegacy Support for MHL_STATUS_PATH_ENABLED\n");
				}
			}
		}
	}
	return retVal;
}

/*
 * SiiMhlTxTmdsEnable
 *
 * Implements conditions on enabling TMDS output stated in MHL spec section 7.6.1
 */
static void SiiMhlTxTmdsEnable(void)
{

	TX_DEBUG_PRINT("MhlTx:SiiMhlTxTmdsEnable\n");
	if (MHL_RSEN & mhlTxConfig.mhlHpdRSENflags) {
		TX_DEBUG_PRINT("\tMHL_RSEN\n");
		if (MHL_HPD & mhlTxConfig.mhlHpdRSENflags) {
			TX_DEBUG_PRINT("\t\tMHL_HPD\n");
			if (QualifyPathEnable()) {
				if (RAP_CONTENT_ON & mhlTxConfig.rapFlags) {
					TX_DEBUG_PRINT("\t\t\t\tRAP CONTENT_ON\n");
					SiiMhlTxDrvTmdsControl(true);
					AppNotifyMhlEvent(MHL_TX_EVENT_TMDS_ENABLED, 0);
				}
			}
		}
	}
}

/*
 * SiiMhlTxSetInt
 *
 * Set MHL defined INTERRUPT bits in peer's register set.
 *
 * This function returns true if operation was successfully performed.
 *
 *  regToWrite      Remote interrupt register to write
 *
 *  mask            the bits to write to that register
 *
 *  priority        0:  add to head of CBusQueue
 *                  1:  add to tail of CBusQueue
 */
static bool_t SiiMhlTxSetInt(uint8_t regToWrite, uint8_t mask,
			     uint8_t priorityLevel)
{
	cbus_req_t req;
	bool_t retVal;

	// find the offset and bit position
	// and feed
	req.retryCount = 2;
	req.command = MHL_SET_INT;
	req.offsetData = regToWrite;
	req.payload_u.msgData[0] = mask;
	if (0 == priorityLevel) {
		retVal = PutPriorityCBusTransaction(&req);
	} else {
		retVal = PutNextCBusTransaction(&req);
	}
	return retVal;
}

/*
 * SiiMhlTxDoWriteBurst
 */
static bool_t SiiMhlTxDoWriteBurst(uint8_t startReg, uint8_t * pData,
				   uint8_t length)
{
	if (FLAGS_WRITE_BURST_PENDING & mhlTxConfig.miscFlags) {
		cbus_req_t req;
		bool_t retVal;

		CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxDoWriteBurst startReg:%d length:%d\n", (int)startReg, (int)length);

		req.retryCount = 1;
		req.command = MHL_WRITE_BURST;
		req.length = length;
		req.offsetData = startReg;
		req.payload_u.pdatabytes = pData;

		retVal = PutPriorityCBusTransaction(&req);
		ClrMiscFlag(SiiMhlTxDoWriteBurst, FLAGS_WRITE_BURST_PENDING)
		    return retVal;
	}
	return false;
}

/*
 * SiiMhlTxInitialize
 *
 * Sets the transmitter component firmware up for operation, brings up chip
 * into power on state first and then back to reduced-power mode D3 to conserve
 * power until an MHL cable connection has been established. If the MHL port is
 * used for USB operation, the chip and firmware continue to stay in D3 mode.
 * Only a small circuit in the chip observes the impedance variations to see if
 * processor should be interrupted to continue MHL discovery process or not.
 *
 * All device events will result in call to the function SiiMhlTxDeviceIsr()
 * by host's hardware or software(a master interrupt handler in host software
 * can call it directly). This implies that the MhlTx component shall make use
 * of AppDisableInterrupts() and AppRestoreInterrupts() for any critical section
 * work to prevent concurrency issues.
 *
 * Parameters
 *
 * pollIntervalMs               This number should be higher than 0 and lower than
 *                                              51 milliseconds for effective operation of the firmware.
 *                                              A higher number will only imply a slower response to an
 *                                              event on MHL side which can lead to violation of a
 *                                              connection disconnection related timing or a slower
 *                                              response to RCP messages.
 */

void SiiMhlTxInitialize(uint8_t pollIntervalMs)
{
	TX_DEBUG_PRINT("MhlTx:SiiMhlTxInitialize\n");

	// Initialize queue of pending CBUS requests.
	CBusQueue.header.head = 0;
	CBusQueue.header.tail = 0;

	//
	// Remember mode of operation.
	//
	mhlTxConfig.pollIntervalMs = pollIntervalMs;

	TX_DEBUG_PRINT("MhlTx: HPD: %d RSEN: %d\n",
			(int)((mhlTxConfig.mhlHpdRSENflags & MHL_HPD) ? 1 : 0)
			,
			(int)((mhlTxConfig.mhlHpdRSENflags & MHL_RSEN) ? 1 : 0)
		       );
	MhlTxResetStates();
	TX_DEBUG_PRINT("MhlTx: HPD: %d RSEN: %d\n",
			(mhlTxConfig.mhlHpdRSENflags & MHL_HPD) ? 1 : 0,
			(mhlTxConfig.mhlHpdRSENflags & MHL_RSEN) ? 1 : 0);

	SiiMhlTxChipInitialize();
}

/*
 * MhlTxProcessEvents
 *
 * This internal function is called at the end of interrupt processing.  It's
 * purpose is to process events detected during the interrupt.  Some events are
 * internally handled here but most are handled by a notification to the application
 * layer.
 */
static void MhlTxProcessEvents(void)
{
	// Make sure any events detected during the interrupt are processed.
	MhlTxDriveStates();
	if (mhlTxConfig.mhlConnectionEvent) {
		TX_DEBUG_PRINT("MhlTx:MhlTxProcessEvents mhlConnectionEvent\n");

		// Consume the message
		mhlTxConfig.mhlConnectionEvent = false;

		//
		// Let app know about the change of the connection state.
		//
		AppNotifyMhlEvent(mhlTxConfig.mhlConnected,
				  mhlTxConfig.mscFeatureFlag);

		// If connection has been lost, reset all state flags.
		if (MHL_TX_EVENT_DISCONNECTION == mhlTxConfig.mhlConnected) {
			MhlTxResetStates();
		} else if (MHL_TX_EVENT_CONNECTION == mhlTxConfig.mhlConnected) {
			SiiMhlTxSetDCapRdy();
		}
	} else if (mhlTxConfig.mscMsgArrived) {
		TX_DEBUG_PRINT("MhlTx:MhlTxProcessEvents MSC MSG <%02X, %02X>\n", (int)(mhlTxConfig.mscMsgSubCommand)
				, (int)(mhlTxConfig.mscMsgData)
			       );

		// Consume the message
		mhlTxConfig.mscMsgArrived = false;

		//
		// Map sub-command to an event id
		//
		switch (mhlTxConfig.mscMsgSubCommand) {
		case MHL_MSC_MSG_RAP:
			// RAP is fully handled here.
			//
			// Handle RAP sub-commands here itself
			//
			if (MHL_RAP_CONTENT_ON == mhlTxConfig.mscMsgData) {
				mhlTxConfig.rapFlags |= RAP_CONTENT_ON;
				TX_DEBUG_PRINT("RAP CONTENT_ON\n");
				SiiMhlTxTmdsEnable();
			} else if (MHL_RAP_CONTENT_OFF ==
				   mhlTxConfig.mscMsgData) {
				mhlTxConfig.rapFlags &= ~RAP_CONTENT_ON;
				TX_DEBUG_PRINT("RAP CONTENT_OFF\n");
				SiiMhlTxDrvTmdsControl(false);
			}
			// Always RAPK to the peer
			SiiMhlTxRapkSend();
			break;

		case MHL_MSC_MSG_RCP:
			// If we get a RCP key that we do NOT support, send back RCPE
			// Do not notify app layer.
			if (MHL_LOGICAL_DEVICE_MAP &
			    rcpSupportTable[mhlTxConfig.mscMsgData & 0x7F]) {
				AppNotifyMhlEvent(MHL_TX_EVENT_RCP_RECEIVED,
						  mhlTxConfig.mscMsgData);
			} else {
				// Save keycode to send a RCPK after RCPE.
				mhlTxConfig.mscSaveRcpKeyCode = mhlTxConfig.mscMsgData;	// key code
				SiiMhlTxRcpeSend(RCPE_INEEFECTIVE_KEY_CODE);
			}
			break;

		case MHL_MSC_MSG_RCPK:
			AppNotifyMhlEvent(MHL_TX_EVENT_RCPK_RECEIVED,
					  mhlTxConfig.mscMsgData);
			DecrementCBusReferenceCount(MhlTxProcessEvents)
			    mhlTxConfig.mscLastCommand = 0;
			mhlTxConfig.mscMsgLastCommand = 0;

			TX_DEBUG_PRINT("MhlTx:MhlTxProcessEvents RCPK\n");
			break;

		case MHL_MSC_MSG_RCPE:
			AppNotifyMhlEvent(MHL_TX_EVENT_RCPE_RECEIVED,
					  mhlTxConfig.mscMsgData);
			break;

		case MHL_MSC_MSG_RAPK:
			// Do nothing if RAPK comes, except decrement the reference counter
			DecrementCBusReferenceCount(MhlTxProcessEvents)
			    mhlTxConfig.mscLastCommand = 0;
			mhlTxConfig.mscMsgLastCommand = 0;
			TX_DEBUG_PRINT("MhlTx:MhlTxProcessEventsRAPK\n");
			break;

		default:
			// Any freak value here would continue with no event to app
			break;
		}
	}
}

/*
 * MhlTxDriveStates
 *
 * This function is called by the interrupt handler in the driver layer.
 * to move the MSC engine to do the next thing before allowing the application
 * to run RCP APIs.
 */
static void MhlTxDriveStates(void)
{
	// process queued CBus transactions
	if (QUEUE_DEPTH(CBusQueue) > 0) {
		if (!SiiMhlTxDrvCBusBusy()) {
			int reQueueRequest = 0;
			cbus_req_t *pReq =
			    GetNextCBusTransaction(MhlTxDriveStates);
			// coordinate write burst requests and grants.
			if (MHL_SET_INT == pReq->command) {
				if (MHL_RCHANGE_INT == pReq->offsetData) {
					if (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.
					    miscFlags) {
						if (MHL_INT_REQ_WRT ==
						    pReq->payload_u.
						    msgData[0]) {
							reQueueRequest = 1;
						} else if (MHL_INT_GRT_WRT ==
							   pReq->payload_u.
							   msgData[0]) {
							reQueueRequest = 0;
						}
					} else {
						if (MHL_INT_REQ_WRT ==
						    pReq->payload_u.
						    msgData[0]) {
							IncrementCBusReferenceCount
							    (MhlTxDriveStates)
							    SetMiscFlag
							    (MhlTxDriveStates,
							     FLAGS_SCRATCHPAD_BUSY)
							    SetMiscFlag
							    (MhlTxDriveStates,
							     FLAGS_WRITE_BURST_PENDING)
						} else if (MHL_INT_GRT_WRT ==
							   pReq->payload_u.
							   msgData[0]) {
							SetMiscFlag
							    (MhlTxDriveStates,
							     FLAGS_SCRATCHPAD_BUSY)
						}
					}
				}
			}
			if (reQueueRequest) {
				// send this one to the back of the line for later attempts
				if (pReq->retryCount-- > 0) {
					PutNextCBusTransaction(pReq);
				}
			} else {
				if (MHL_MSC_MSG == pReq->command) {
					mhlTxConfig.mscMsgLastCommand =
					    pReq->payload_u.msgData[0];
					mhlTxConfig.mscMsgLastData =
					    pReq->payload_u.msgData[1];
				} else {
					mhlTxConfig.mscLastOffset =
					    pReq->offsetData;
					mhlTxConfig.mscLastData =
					    pReq->payload_u.msgData[0];

				}
				mhlTxConfig.mscLastCommand = pReq->command;

				IncrementCBusReferenceCount(MhlTxDriveStates)
				    SiiMhlTxDrvSendCbusCommand(pReq);
			}
		}
	}
}

static void ExamineLocalAndPeerVidLinkMode(void)
{
	// set default values
	mhlTxConfig.linkMode &= ~MHL_STATUS_CLK_MODE_MASK;
	mhlTxConfig.linkMode |= MHL_STATUS_CLK_MODE_NORMAL;

	// when more modes than PPIXEL and normal are supported,
	//   this should become a table lookup.
	if (MHL_DEV_VID_LINK_SUPP_PPIXEL & mhlTxConfig.
	    aucDevCapCache[DEVCAP_OFFSET_VID_LINK_MODE]) {
		if (MHL_DEV_VID_LINK_SUPP_PPIXEL & DEVCAP_VAL_VID_LINK_MODE) {
			mhlTxConfig.linkMode &= ~MHL_STATUS_CLK_MODE_MASK;
			mhlTxConfig.linkMode |= mhlTxConfig.preferredClkMode;
		}
	}
	// call driver here to set CLK_MODE
	SiMhlTxDrvSetClkMode(mhlTxConfig.linkMode & MHL_STATUS_CLK_MODE_MASK);
}

// -- mw20110614 for dongle power detection
#define WriteByteCBUS(offset,value)  SiiRegWrite(TX_PAGE_CBUS | (uint16_t)offset , value)
#define ReadByteCBUS(offset)         SiiRegRead(TX_PAGE_CBUS | (uint16_t)offset)

static int charger_check_state = -1;
void Enable_Backdoor_Access(void)
{
	WriteByteCBUS(0x13, 0x33);         // enable backdoor access
	WriteByteCBUS(0x14, 0x80);
	WriteByteCBUS(0x12, 0x08);
}
static void Disable_Backdoor_Access(void)
{
	WriteByteCBUS(0x20, 0x02);       // burst length-1
	WriteByteCBUS(0x13, 0x48);       // offset in scratch pad
	WriteByteCBUS(0x12, 0x10);       // trig this cmd
	WriteByteCBUS(0x13, 0x33);         // enable backdoor access
	WriteByteCBUS(0x14, 0x00);
	WriteByteCBUS(0x12, 0x08);
}
static void Tri_state_dongle_GPIO0(void)
{
	Enable_Backdoor_Access();
	WriteByteCBUS(0xc0, 0xff);        // main page , FE for Cbus page
	WriteByteCBUS(0xc1, 0x7f);        // offset
	WriteByteCBUS(0xc2, 0xff);        // data set GPIO=input
	Disable_Backdoor_Access();
}
static void Low_dongle_GPIO0(void)
{
	Enable_Backdoor_Access();
	WriteByteCBUS(0xc0, 0xff);        // main page , FE for Cbus page
	WriteByteCBUS(0xc1, 0x7f);        // offset
	WriteByteCBUS(0xc2, 0xfc);        // data set GPIO=input
	Disable_Backdoor_Access();
}

static int charger_check(uint8_t data1)
{
	if ((data1) == 0x00) {
	/* mw20110811 First ReadDevCap(02) always get return with CbusReg0x16=00;
	 * mw20110826 but if data1=00, it's not TV, not dongle,
	 * should no need to qualify
	 */
		charger_check_state = -1;
		return TYPE_NONE;
	}

	TX_DEBUG_PRINT("CbusReg0x16=%02XH", (int)data1);
	if (charger_check_state == 0) {
		if ((data1 & 0x13) == 0x11) {
			TX_DEBUG_PRINT("Rx is TV, off Vbus out, set charge current=500mA\n");
			charger_check_state = -1;
			return TYPE_USB;
		} else if ((data1 & 0x03) == 0x03) {
			//03=dongle , must & with 0x03, due to POW=0 if USB attached, and GPIO0=low
			TX_DEBUG_PRINT("3 state GPIO0\n");
			Tri_state_dongle_GPIO0();
			SiiMhlTxReadDevcap(2);
			charger_check_state = 1;
			return TYPE_NONE;
		}
	} else if (charger_check_state == 1) {
		if (data1 & 0x10) {
			// POW bit=1=Rx has power
			TX_DEBUG_PRINT(" POW=1=Charger port has power,low GPIO0\n");

			//Turn off phone Vbus output ;
			Low_dongle_GPIO0();
			/*eadDevCapReg0x02 packet out;
			 *POW will be returned on next MhlCbusIsr( )
			 */
			SiiMhlTxReadDevcap(2);
			charger_check_state = 2;
		} else{
			TX_DEBUG_PRINT("Dongle has no power,enable 5V to Vbus \n");
			charger_check_state = -1;
			return TYPE_NO_POWER;
		}
	} else if (charger_check_state == 2) {
		charger_check_state = -1;
		if (data1 & 0x10) {
			//[bit4] POW ==1=AC charger attached
			TX_DEBUG_PRINT("set charge current=AC\n");
			mhl_enable_charging(POWER_SUPPLY_TYPE_MAINS);
			return TYPE_AC;
		} else {
			//charger port only has USB source provide 5V/100mA,
			//that just enough dongle to work, no more current to charge phone battery
			TX_DEBUG_PRINT("set charge current=USB\n");
			//at least no need to send out 5V/100mA power
			mhl_enable_charging(POWER_SUPPLY_TYPE_USB);
			Tri_state_dongle_GPIO0();
			return TYPE_USB;
		}
	} else {
		pr_info("charger check function error!\n");
		charger_check_state = -1;
	}
	return TYPE_NONE;
}

int setting_external_charger(void)
{
	charger_check_state = 0;
	SiiMhlTxReadDevcap(2);
	MhlTxDriveStates();
	return 0;
}

/*
 * SiiMhlTxMscCommandDone
 *
 * This function is called by the driver to inform of completion of last command.
 *
 * It is called in interrupt context to meet some MHL specified timings, therefore,
 * it should not have to call app layer and do negligible processing, no printfs.
 */
#define FLAG_OR_NOT(x) (FLAGS_HAVE_##x & mhlTxConfig.miscFlags)?#x:""
#define SENT_OR_NOT(x) (FLAGS_SENT_##x & mhlTxConfig.miscFlags)?#x:""

static void SiiMhlTxMscCommandDone(uint8_t data1)
{
	CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscCommandDone. data1 = %02X\n",
			(int)data1);

	DecrementCBusReferenceCount(SiiMhlTxMscCommandDone)
	if (MHL_READ_DEVCAP == mhlTxConfig.mscLastCommand) {
		if (mhlTxConfig.mscLastOffset <
		    sizeof(mhlTxConfig.aucDevCapCache)) {
			// populate the cache
			mhlTxConfig.aucDevCapCache[mhlTxConfig.mscLastOffset] =
			    data1;
			CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscCommandDone peer DEV_CAP[0x%02x]:0x%02x index:0x%02x\n",
					(int)mhlTxConfig.mscLastOffset, (int)data1, (int)mhlTxConfig.ucDevCapCacheIndex);

			if (MHL_DEV_CATEGORY_OFFSET ==
			    mhlTxConfig.mscLastOffset) {
				uint8_t param;

				if (charger_check_state >= 0)
					charger_check(data1);

				mhlTxConfig.miscFlags |=
				    FLAGS_HAVE_DEV_CATEGORY;
				param = data1 & MHL_DEV_CATEGORY_POW_BIT;
				CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscCommandDone FLAGS_HAVE_DEV_CATEGORY\n");
			} else if (MHL_DEV_FEATURE_FLAG_OFFSET ==
				   mhlTxConfig.mscLastOffset) {
				mhlTxConfig.miscFlags |=
				    FLAGS_HAVE_DEV_FEATURE_FLAGS;
				CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscCommandDone FLAGS_HAVE_DEV_FEATURE_FLAGS\n");

				// Remember features of the peer
				mhlTxConfig.mscFeatureFlag = data1;

				CBUS_DEBUG_PRINT("MhlTx: Peer's Feature Flag = %02X\n\n", (int)data1);
			} else if (DEVCAP_OFFSET_VID_LINK_MODE ==
				   mhlTxConfig.mscLastOffset) {
				ExamineLocalAndPeerVidLinkMode();
			}

			if (++mhlTxConfig.ucDevCapCacheIndex <
			    sizeof(mhlTxConfig.aucDevCapCache)) {
				// OK to call this here, since requests always get queued and processed in the "foreground"
				SiiMhlTxReadDevcap(mhlTxConfig.
						   ucDevCapCacheIndex);
			} else {
				// this is necessary for both firmware and linux driver.

				//Only if PATH_EN is not already set.
				if (!(MHL_STATUS_PATH_ENABLED & mhlTxConfig.status_1)) {
					SiiMhlTxTmdsEnable();
				}
				AppNotifyMhlEvent(MHL_TX_EVENT_DCAP_CHG, 0);

				// These variables are used to remember if we issued a READ_DEVCAP
				//    or other MSC command
				// Since we are done, reset them.
				mhlTxConfig.mscLastCommand = 0;
				mhlTxConfig.mscLastOffset = 0;
			}
		}
	} else if (MHL_WRITE_STAT == mhlTxConfig.mscLastCommand) {

		CBUS_DEBUG_PRINT("MhlTx: WRITE_STAT miscFlags: %02X\n\n",
				(int)mhlTxConfig.miscFlags);
		if (MHL_STATUS_REG_CONNECTED_RDY == mhlTxConfig.mscLastOffset) {
			if (MHL_STATUS_DCAP_RDY & mhlTxConfig.mscLastData) {
				mhlTxConfig.miscFlags |= FLAGS_SENT_DCAP_RDY;
				CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscCommandDone FLAGS_SENT_DCAP_RDY\n");
				SiiMhlTxSetInt(MHL_RCHANGE_INT, MHL_INT_DCAP_CHG, 0);	// priority request
			}
		} else if (MHL_STATUS_REG_LINK_MODE ==
			   mhlTxConfig.mscLastOffset) {
			if (MHL_STATUS_PATH_ENABLED & mhlTxConfig.mscLastData) {
				mhlTxConfig.miscFlags |= FLAGS_SENT_PATH_EN;
				CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscCommandDone FLAGS_SENT_PATH_EN\n");
			}
		}

		mhlTxConfig.mscLastCommand = 0;
		mhlTxConfig.mscLastOffset = 0;
	} else if (MHL_MSC_MSG == mhlTxConfig.mscLastCommand) {
		if (SiiMhlTxDrvMscMsgNacked()) {
			cbus_req_t retry;
			retry.retryCount = 2;
			retry.command = mhlTxConfig.mscLastCommand;
			retry.payload_u.msgData[0] = mhlTxConfig.mscMsgLastCommand;
			retry.payload_u.msgData[1] = mhlTxConfig.mscMsgLastData;
			SiiMhlTxDrvSendCbusCommand( &retry );
		} else {
			if (MHL_MSC_MSG_RCPE == mhlTxConfig.mscMsgLastCommand) {
				//
				// RCPE is always followed by an RCPK with original key code that came.
				//
				if(SiiMhlTxRcpkSend(mhlTxConfig.mscSaveRcpKeyCode))
				{
				}
			} else {
				CBUS_DEBUG_PRINT (("MhlTx:SiiMhlTxMscCommandDone default\n"
					"\tmscLastCommand: 0x%02X \n"
					"\tmscMsgLastCommand: 0x%02X mscMsgLastData: 0x%02X\n"
					"\tcbusReferenceCount: %d\n"
					,(int)mhlTxConfig.mscLastCommand
					,(int)mhlTxConfig.mscMsgLastCommand
					,(int)mhlTxConfig.mscMsgLastData
					,(int)mhlTxConfig.cbusReferenceCount));
			}
			mhlTxConfig.mscLastCommand = 0;
		}
	} else if (MHL_WRITE_BURST == mhlTxConfig.mscLastCommand) {
		CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscCommandDone MHL_WRITE_BURST\n");
		mhlTxConfig.mscLastCommand = 0;
		mhlTxConfig.mscLastOffset = 0;
		mhlTxConfig.mscLastData = 0;

		// all CBus request are queued, so this is OK to call here
		// use priority 0 so that other queued commands don't interfere
		SiiMhlTxSetInt(MHL_RCHANGE_INT, MHL_INT_DSCR_CHG, 0);
	} else if (MHL_SET_INT == mhlTxConfig.mscLastCommand) {
		CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscCommandDone MHL_SET_INT\n");
		if (MHL_RCHANGE_INT == mhlTxConfig.mscLastOffset) {
			CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscCommandDone MHL_RCHANGE_INT\n");
			if (MHL_INT_DSCR_CHG == mhlTxConfig.mscLastData) {
				DecrementCBusReferenceCount(SiiMhlTxMscCommandDone)	// this one is for the write burst request
				CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscCommandDone MHL_INT_DSCR_CHG\n");
				ClrMiscFlag(SiiMhlTxMscCommandDone,
					    FLAGS_SCRATCHPAD_BUSY)
			}
		}
		// Once the command has been sent out successfully, forget this case.
		mhlTxConfig.mscLastCommand = 0;
		mhlTxConfig.mscLastOffset = 0;
		mhlTxConfig.mscLastData = 0;
	} else {
		CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscCommandDone default\n"
				"\tmscLastCommand: 0x%02X mscLastOffset: 0x%02X\n"
				"\tcbusReferenceCount: %d\n",
				(int)mhlTxConfig.mscLastCommand,
				(int)mhlTxConfig.mscLastOffset,
				(int)mhlTxConfig.cbusReferenceCount);
	}
	if (!(FLAGS_RCP_READY & mhlTxConfig.miscFlags)) {
		CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscCommandDone. have(%s %s) sent(%s %s)\n", FLAG_OR_NOT(DEV_CATEGORY)
				, FLAG_OR_NOT(DEV_FEATURE_FLAGS)
				, SENT_OR_NOT(PATH_EN)
				, SENT_OR_NOT(DCAP_RDY)
			       );
		if (FLAGS_HAVE_DEV_CATEGORY & mhlTxConfig.miscFlags) {
			if (FLAGS_HAVE_DEV_FEATURE_FLAGS & mhlTxConfig.
			    miscFlags) {
				if (FLAGS_SENT_PATH_EN & mhlTxConfig.miscFlags) {
					if (FLAGS_SENT_DCAP_RDY & mhlTxConfig.
					    miscFlags) {
						if (mhlTxConfig.
						    ucDevCapCacheIndex >=
						    sizeof(mhlTxConfig.
							   aucDevCapCache)) {
							mhlTxConfig.miscFlags |=
							    FLAGS_RCP_READY;
							// Now we can entertain App commands for RCP
							// Let app know this state
							mhlTxConfig.
							    mhlConnectionEvent =
							    true;
							mhlTxConfig.
							    mhlConnected =
							    MHL_TX_EVENT_RCP_READY;
						}
					}
				}
			}
		}
	}
}

/*
 * SiiMhlTxMscWriteBurstDone
 *
 * This function is called by the driver to inform of completion of a write burst.
 *
 * It is called in interrupt context to meet some MHL specified timings, therefore,
 * it should not have to call app layer and do negligible processing, no printfs.
 */
static void SiiMhlTxMscWriteBurstDone(uint8_t data1)
{
#define WRITE_BURST_TEST_SIZE 16
	uint8_t temp[WRITE_BURST_TEST_SIZE];
	uint8_t i;
	data1 = data1;		// make this compile for NON debug builds
	CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxMscWriteBurstDone(%02X) \"",
			(int)data1);
	SiiMhlTxDrvGetScratchPad(0, temp, WRITE_BURST_TEST_SIZE);
	for (i = 0; i < WRITE_BURST_TEST_SIZE; ++i) {
		if (temp[i] >= ' ') {
			CBUS_DEBUG_PRINT("%02X %c ", (int)temp[i], temp[i]);
		} else {
			CBUS_DEBUG_PRINT("%02X . ", (int)temp[i]);
		}
	}
	CBUS_DEBUG_PRINT("\"\n");
}

/*
 * SiiMhlTxGotMhlMscMsg
 *
 * This function is called by the driver to inform of arrival of a MHL MSC_MSG
 * such as RCP, RCPK, RCPE. To quickly return back to interrupt, this function
 * remembers the event (to be picked up by app later in task context).
 *
 * It is called in interrupt context to meet some MHL specified timings, therefore,
 * it should not have to call app layer and do negligible processing of its own,
 *
 * No printfs.
 *
 * Application shall not call this function.
 */
static void SiiMhlTxGotMhlMscMsg(uint8_t subCommand, uint8_t cmdData)
{
	// Remember the event.
	mhlTxConfig.mscMsgArrived = true;
	mhlTxConfig.mscMsgSubCommand = subCommand;
	mhlTxConfig.mscMsgData = cmdData;
}

/*
 *
 * SiiMhlTxGotMhlIntr
 *
 * This function is called by the driver to inform of arrival of a MHL INTERRUPT.
 *
 * It is called in interrupt context to meet some MHL specified timings, therefore,
 * it should not have to call app layer and do negligible processing, no printfs.
 *
 */
static void SiiMhlTxGotMhlIntr(uint8_t intr_0, uint8_t intr_1)
{
	TX_DEBUG_PRINT("MhlTx: INTERRUPT Arrived. %02X, %02X\n", (int)intr_0,
			(int)intr_1);

	//
	// Handle DCAP_CHG INTR here
	//
	if (MHL_INT_DCAP_CHG & intr_0) {
		if (MHL_STATUS_DCAP_RDY & mhlTxConfig.status_0) {
			SiiMhlTxRefreshPeerDevCapEntries();
		}
	}

	if (MHL_INT_DSCR_CHG & intr_0) {
		SiiMhlTxDrvGetScratchPad(0, mhlTxConfig.localScratchPad,
					 sizeof(mhlTxConfig.localScratchPad));
		// remote WRITE_BURST is complete
		ClrMiscFlag(SiiMhlTxGotMhlIntr, FLAGS_SCRATCHPAD_BUSY)
		    AppNotifyMhlEvent(MHL_TX_EVENT_DSCR_CHG, 0);
	}
	if (MHL_INT_REQ_WRT & intr_0) {

		// this is a request from the sink device.
		if (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags) {
			// use priority 1 to defer sending grant until
			//  local traffic is done
			SiiMhlTxSetInt(MHL_RCHANGE_INT, MHL_INT_GRT_WRT, 1);
		} else {
			SetMiscFlag(SiiMhlTxGotMhlIntr, FLAGS_SCRATCHPAD_BUSY)
			    // OK to call this here, since all requests are queued
			    // use priority 0 to respond immediately
			    SiiMhlTxSetInt(MHL_RCHANGE_INT, MHL_INT_GRT_WRT, 0);
		}
	}
	if (MHL_INT_GRT_WRT & intr_0) {
		uint8_t length = sizeof(mhlTxConfig.localScratchPad);
		TX_DEBUG_PRINT("MhlTx: MHL_INT_GRT_WRT length:%d\n",
				(int)length);
		SiiMhlTxDoWriteBurst(0x40, mhlTxConfig.localScratchPad, length);
	}
	// removed "else", since interrupts are not mutually exclusive of each other.
	if (MHL_INT_EDID_CHG & intr_1) {
		// force upstream source to read the EDID again.
		// Most likely by appropriate togggling of HDMI HPD
		SiiMhlTxDrvNotifyEdidChange();
	}
}

/*
 *SiiMhlTxGotMhlStatus
 *
 * This function is called by the driver to inform of arrival of a MHL STATUS.
 *
 * It is called in interrupt context to meet some MHL specified timings, therefore,
 * it should not have to call app layer and do negligible processing, no printfs.
 */
static void SiiMhlTxGotMhlStatus(uint8_t status_0, uint8_t status_1)
{
//      TX_DEBUG_PRINT("MhlTx: STATUS Arrived. %02X, %02X\n", (int) status_0, (int) status_1);
	//
	// Handle DCAP_RDY STATUS here itself
	//
	uint8_t StatusChangeBitMask0, StatusChangeBitMask1;
	StatusChangeBitMask0 = status_0 ^ mhlTxConfig.status_0;
	StatusChangeBitMask1 = status_1 ^ mhlTxConfig.status_1;
	// Remember the event.
	// (other code checks the saved values, so save the values early, but not before the XOR operations above)
	mhlTxConfig.status_0 = status_0;
	mhlTxConfig.status_1 = status_1;

	if (MHL_STATUS_DCAP_RDY & StatusChangeBitMask0) {

		TX_DEBUG_PRINT("MhlTx: DCAP_RDY changed\n");
		if (MHL_STATUS_DCAP_RDY & status_0) {
			SiiMhlTxRefreshPeerDevCapEntries();
		}
	}

	// did PATH_EN change?
	if (MHL_STATUS_PATH_ENABLED & StatusChangeBitMask1) {
		TX_DEBUG_PRINT("MhlTx: PATH_EN changed\n");
		if (MHL_STATUS_PATH_ENABLED & status_1) {
			// OK to call this here since all requests are queued
			SiiMhlTxSetPathEn();
		} else {
			// OK to call this here since all requests are queued
			SiiMhlTxClrPathEn();
		}
	}

}

/*
 *
 * SiiMhlTxRcpkSend
 *
 * This function sends RCPK to the peer device.
 *
 */
static bool_t SiiMhlTxRcpkSend(uint8_t rcpKeyCode)
{
	bool_t retVal;

	retVal = MhlTxSendMscMsg(MHL_MSC_MSG_RCPK, rcpKeyCode);
	if (retVal) {
		MhlTxDriveStates();
	}
	return retVal;
}

/*
 *
 * SiiMhlTxRapkSend
 *
 * This function sends RAPK to the peer device.
 *
 */
static bool_t SiiMhlTxRapkSend(void)
{
	return (MhlTxSendMscMsg(MHL_MSC_MSG_RAPK, 0));
}

/*
 * SiiMhlTxRcpeSend
 *
 * The function will return a value of true if it could successfully send the RCPE
 * subcommand. Otherwise false.
 *
 * When successful, MhlTx internally sends RCPK with original (last known)
 * keycode.
 */
static bool_t SiiMhlTxRcpeSend(uint8_t rcpeErrorCode)
{
	bool_t retVal;

	retVal = MhlTxSendMscMsg(MHL_MSC_MSG_RCPE, rcpeErrorCode);
	if (retVal) {
		MhlTxDriveStates();
	}
	return retVal;
}

/*
 * SiiMhlTxSetStatus
 *
 * Set MHL defined STATUS bits in peer's register set.
 *
 * register         MHLRegister to write
 *
 * value        data to write to the register
 *
 */

static bool_t SiiMhlTxSetStatus(uint8_t regToWrite, uint8_t value)
{
	cbus_req_t req;
	bool_t retVal;

	// find the offset and bit position
	// and feed
	req.retryCount = 2;
	req.command = MHL_WRITE_STAT;
	req.offsetData = regToWrite;
	req.payload_u.msgData[0] = value;

	TX_DEBUG_PRINT("MhlTx:SiiMhlTxSetStatus (%02x, %02x)\n", (int)regToWrite, (int)value);
	retVal = PutNextCBusTransaction(&req);
	return retVal;
}

/*
 * SiiMhlTxSetDCapRdy
 */
static bool_t SiiMhlTxSetDCapRdy(void)
{
	mhlTxConfig.connectedReady |= MHL_STATUS_DCAP_RDY;	// update local copy
	return SiiMhlTxSetStatus(MHL_STATUS_REG_CONNECTED_RDY,
				 mhlTxConfig.connectedReady);
}

/*
 * SiiMhlTxSetPathEn
 */
static bool_t SiiMhlTxSetPathEn(void)
{
	TX_DEBUG_PRINT("MhlTx:SiiMhlTxSetPathEn\n");
	SiiMhlTxTmdsEnable();
	mhlTxConfig.linkMode |= MHL_STATUS_PATH_ENABLED;	// update local copy
	return SiiMhlTxSetStatus(MHL_STATUS_REG_LINK_MODE,
				 mhlTxConfig.linkMode);
}

/*
 * SiiMhlTxClrPathEn
 */
static bool_t SiiMhlTxClrPathEn(void)
{
	TX_DEBUG_PRINT("MhlTx: SiiMhlTxClrPathEn\n");
	SiiMhlTxDrvTmdsControl(false);
	mhlTxConfig.linkMode &= ~MHL_STATUS_PATH_ENABLED;	// update local copy
	return SiiMhlTxSetStatus(MHL_STATUS_REG_LINK_MODE,
				 mhlTxConfig.linkMode);
}

/*
 * SiiMhlTxReadDevcap
 *
 * This function sends a READ DEVCAP MHL command to the peer.
 * It  returns true if successful in doing so.
 *
 * The value of devcap should be obtained by making a call to SiiMhlTxGetEvents()
 *
 * offset               Which byte in devcap register is required to be read. 0..0x0E
 */
static bool_t SiiMhlTxReadDevcap(uint8_t offset)
{
	cbus_req_t req;
	CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxReadDevcap\n");
	//
	// Send MHL_READ_DEVCAP command
	//
	req.retryCount = 2;
	req.command = MHL_READ_DEVCAP;
	req.offsetData = offset;
	req.payload_u.msgData[0] = 0;	// do this to avoid confusion

	return PutNextCBusTransaction(&req);
}

/*
 * SiiMhlTxRefreshPeerDevCapEntries
 */

static void SiiMhlTxRefreshPeerDevCapEntries(void)
{
	// only issue if no existing refresh is in progress
	if (mhlTxConfig.ucDevCapCacheIndex >=
	    sizeof(mhlTxConfig.aucDevCapCache)) {
		mhlTxConfig.ucDevCapCacheIndex = 0;
		SiiMhlTxReadDevcap(mhlTxConfig.ucDevCapCacheIndex);
	}
}

/*
 * MhlTxSendMscMsg
 *
 * This function sends a MSC_MSG command to the peer.
 * It  returns true if successful in doing so.
 *
 * The value of devcap should be obtained by making a call to SiiMhlTxGetEvents()
 *
 * offset               Which byte in devcap register is required to be read. 0..0x0E
 */
static bool_t MhlTxSendMscMsg(uint8_t command, uint8_t cmdData)
{
	cbus_req_t req;
	uint8_t ccode;

	//
	// Send MSC_MSG command
	//
	// Remember last MSC_MSG command (RCPE particularly)
	//
	req.retryCount = 2;
	req.command = MHL_MSC_MSG;
	req.payload_u.msgData[0] = command;
	req.payload_u.msgData[1] = cmdData;
	ccode = PutNextCBusTransaction(&req);
	return ((bool_t) ccode);
}

/*
 * SiiMhlTxNotifyConnection
 */
static void SiiMhlTxNotifyConnection(bool_t mhlConnected)
{
	mhlTxConfig.mhlConnectionEvent = true;

	TX_DEBUG_PRINT("MhlTx: SiiMhlTxNotifyConnection MSC_STATE_IDLE %01X\n",
			(int)mhlConnected);

	if (mhlConnected) {
		mhlTxConfig.rapFlags |= RAP_CONTENT_ON;
		TX_DEBUG_PRINT("RAP CONTENT_ON\n");
		mhlTxConfig.mhlConnected = MHL_TX_EVENT_CONNECTION;
		mhlTxConfig.mhlHpdRSENflags |= MHL_RSEN;
		SiiMhlTxTmdsEnable();

		// Link mode must not be sent at this time. It should only be sent as a response to
		// SET_PATH_EN, CLR_PATH_EN,or a change in CLK_MODE on the link. Sending a CLK_MODE
		// of 3 at this time is probably okay for MHL compliant devices as this should be a
		// default setting. However, sending this to a legacy device could have undesirable
		// results.
        //SiiMhlTxSendLinkMode();
	} else {
		mhlTxConfig.rapFlags &= ~RAP_CONTENT_ON;
		TX_DEBUG_PRINT("RAP CONTENT_OFF\n");
		mhlTxConfig.mhlConnected = MHL_TX_EVENT_DISCONNECTION;
		mhlTxConfig.mhlHpdRSENflags &= ~MHL_RSEN;
	}
	MhlTxProcessEvents();
}

/*
 * SiiMhlTxNotifyDsHpdChange
 * Driver tells about arrival of SET_HPD or CLEAR_HPD by calling this function.
 *
 * Turn the content off or on based on what we got.
 */
static void SiiMhlTxNotifyDsHpdChange(uint8_t dsHpdStatus)
{
	if (0 == dsHpdStatus) {
		TX_DEBUG_PRINT("MhlTx: Disable TMDS\n");
		TX_DEBUG_PRINT("MhlTx: DsHPD OFF\n");
		mhlTxConfig.mhlHpdRSENflags &= ~MHL_HPD;
		AppNotifyMhlDownStreamHPDStatusChange(dsHpdStatus);
		SiiMhlTxDrvTmdsControl(false);
	} else {
		TX_DEBUG_PRINT("MhlTx: Enable TMDS\n");
		TX_DEBUG_PRINT("MhlTx: DsHPD ON\n");
		mhlTxConfig.mhlHpdRSENflags |= MHL_HPD;
		AppNotifyMhlDownStreamHPDStatusChange(dsHpdStatus);
		SiiMhlTxTmdsEnable();
	}
}


/*
 * MhlTxResetStates
 *
 * Application picks up mhl connection and rcp events at periodic intervals.
 * Interrupt handler feeds these variables. Reset them on disconnection.
 */
static void MhlTxResetStates(void)
{
	mhlTxConfig.mhlConnectionEvent = false;
	mhlTxConfig.mhlConnected = MHL_TX_EVENT_DISCONNECTION;
	mhlTxConfig.mhlHpdRSENflags &= ~(MHL_RSEN | MHL_HPD);
	mhlTxConfig.rapFlags &= ~RAP_CONTENT_ON;
	TX_DEBUG_PRINT("RAP CONTENT_OFF\n");
	mhlTxConfig.mscMsgArrived = false;

	mhlTxConfig.status_0 = 0;
	mhlTxConfig.status_1 = 0;
	mhlTxConfig.connectedReady = 0;
	mhlTxConfig.linkMode = MHL_STATUS_CLK_MODE_NORMAL;	// indicate normal (24-bit) mode
	mhlTxConfig.preferredClkMode = MHL_STATUS_CLK_MODE_NORMAL;	// this can be overridden by the application calling SiiMhlTxSetPreferredPixelFormat()
	mhlTxConfig.cbusReferenceCount = 0;
	mhlTxConfig.miscFlags = 0;
	mhlTxConfig.mscLastCommand = 0;
	mhlTxConfig.mscMsgLastCommand = 0;
	mhlTxConfig.ucDevCapCacheIndex = 1 + sizeof(mhlTxConfig.aucDevCapCache);
}

static void SiiMhlTxNotifyRgndMhl(void)
{
	// Application layer did not claim this, so send it to the driver layer
	SiiMhlTxDrvProcessRgndMhl();
}

#if 0
/*
    SiiTxReadConnectionStatus
    returns:
    0: if not fully connected
    1: if fully connected
*/
static uint8_t SiiTxReadConnectionStatus(void)
{
	return (mhlTxConfig.mhlConnected >= MHL_TX_EVENT_RCP_READY) ? 1 : 0;
}

/*
  SiiMhlTxSetPreferredPixelFormat

	clkMode - the preferred pixel format for the CLK_MODE status register

	Returns: 0 -- success
		     1 -- failure - bits were specified that are not within the mask
 */
static uint8_t SiiMhlTxSetPreferredPixelFormat(uint8_t clkMode)
{
	if (~MHL_STATUS_CLK_MODE_MASK & clkMode) {
		return 1;
	} else {
		mhlTxConfig.preferredClkMode = clkMode;

		// check to see if a refresh has happened since the last call
		//   to MhlTxResetStates()
		if (mhlTxConfig.ucDevCapCacheIndex <=
		    sizeof(mhlTxConfig.aucDevCapCache)) {
			// check to see if DevCap cache update has already updated this yet.
			if (mhlTxConfig.ucDevCapCacheIndex >
			    DEVCAP_OFFSET_VID_LINK_MODE) {
				ExamineLocalAndPeerVidLinkMode();
			}
		}
		return 0;
	}
}

/*
	SiiTxGetPeerDevCapEntry
	index -- the devcap index to get
	*pData pointer to location to write data
	returns
		0 -- success
		1 -- busy.
 */
static uint8_t SiiTxGetPeerDevCapEntry(uint8_t index, uint8_t * pData)
{
	if (mhlTxConfig.ucDevCapCacheIndex < sizeof(mhlTxConfig.aucDevCapCache)) {
		// update is in progress
		return 1;
	} else {
		*pData = mhlTxConfig.aucDevCapCache[index];
		return 0;
	}
}

/*
	SiiGetScratchPadVector
	offset -- The beginning offset into the scratch pad from which to fetch entries.
	length -- The number of entries to fetch
	*pData -- A pointer to an array of bytes where the data should be placed.

	returns:
		ScratchPadStatus_e see si_mhl_tx_api.h for details

*/
static ScratchPadStatus_e SiiGetScratchPadVector(uint8_t offset, uint8_t length,
					  uint8_t * pData)
{
	if (!(MHL_FEATURE_SP_SUPPORT & mhlTxConfig.mscFeatureFlag)) {
		TX_DEBUG_PRINT("MhlTx:SiiMhlTxRequestWriteBurst failed SCRATCHPAD_NOT_SUPPORTED\n");
		return SCRATCHPAD_NOT_SUPPORTED;
	} else if (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags) {
		return SCRATCHPAD_BUSY;
	} else if ((offset >= sizeof(mhlTxConfig.localScratchPad)) ||
			(length > (sizeof(mhlTxConfig.localScratchPad) - offset))) {
		return SCRATCHPAD_BAD_PARAM;
	} else {
		uint8_t i, reg;
		for (i = 0, reg = offset;
		     (i < length)
		     && (reg < sizeof(mhlTxConfig.localScratchPad));
		     ++i, ++reg) {
			pData[i] = mhlTxConfig.localScratchPad[reg];
		}
		return SCRATCHPAD_SUCCESS;
	}
}

static uint16_t SiiMhlTxDrvGetDeviceId(void)
{
	uint16_t retVal;
	retVal =  SiiRegRead(REG_DEV_IDH);
	retVal <<= 8;
	retVal |= SiiRegRead(REG_DEV_IDL);
	return retVal;
}

static uint8_t SiiMhlTxDrvGetDeviceRev(void)
{
	return SiiRegRead(REG_DEV_REV);
}

/*
 * SiiMhlTxDrvPowBitChange
 *
 * Alert the driver that the peer's POW bit has changed so that it can take
 * action if necessary.
 *
 */
static void SiiMhlTxDrvPowBitChange(bool_t enable)
{
	// MHL peer device has it's own power
	if (enable) {
		SiiRegModify(REG_DISC_CTRL8, 0x04, 0x04);
		TX_DEBUG_PRINT("Drv: POW bit 0->1, set DISC_CTRL8[2] = 1\n");
	}
}

static bool_t MhlTxCBusBusy(void)
{
	return ((QUEUE_DEPTH(CBusQueue) > 0) || SiiMhlTxDrvCBusBusy()
		|| mhlTxConfig.cbusReferenceCount) ? true : false;
}

/*
 * SiiMhlTxRequestWriteBurst
 */
static ScratchPadStatus_e SiiMhlTxRequestWriteBurst(uint8_t startReg, uint8_t length,
					     uint8_t * pData)
{
	ScratchPadStatus_e retVal = SCRATCHPAD_BUSY;

	if (!(MHL_FEATURE_SP_SUPPORT & mhlTxConfig.mscFeatureFlag)) {
		CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxRequestWriteBurst failed SCRATCHPAD_NOT_SUPPORTED\n");
		retVal = SCRATCHPAD_NOT_SUPPORTED;
	} else {
		if ((FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags)
		    || MhlTxCBusBusy()
		    ) {
			CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxRequestWriteBurst failed FLAGS_SCRATCHPAD_BUSY \n");
		} else {
			bool_t temp;
			uint8_t i, reg;
			CBUS_DEBUG_PRINT("MhlTx:SiiMhlTxRequestWriteBurst, request sent\n");
			for (i = 0, reg = startReg;
			     (i < length) && (reg < SCRATCHPAD_SIZE);
			     ++i, ++reg) {
				mhlTxConfig.localScratchPad[reg] = pData[i];
			}

			temp =
			    SiiMhlTxSetInt(MHL_RCHANGE_INT, MHL_INT_REQ_WRT, 1);
			retVal = temp ? SCRATCHPAD_SUCCESS : SCRATCHPAD_FAIL;
		}
	}
	return retVal;
}

/*
 *
 * SiiMhlTxRcpSend
 *
 * This function checks if the peer device supports RCP and sends rcpKeyCode. The
 * function will return a value of true if it could successfully send the RCP
 * subcommand and the key code. Otherwise false.
 *
 * The followings are not yet utilized.
 *
 * (MHL_FEATURE_RAP_SUPPORT & mhlTxConfig.mscFeatureFlag))
 * (MHL_FEATURE_SP_SUPPORT & mhlTxConfig.mscFeatureFlag))
 *
 */
static bool_t SiiMhlTxRcpSend(uint8_t rcpKeyCode)
{
	bool_t retVal;
	//
	// If peer does not support do not send RCP or RCPK/RCPE commands
	//

	if ((0 == (MHL_FEATURE_RCP_SUPPORT & mhlTxConfig.mscFeatureFlag))
	    || !(FLAGS_RCP_READY & mhlTxConfig.miscFlags)
	    ) {
		TX_DEBUG_PRINT("MhlTx:SiiMhlTxRcpSend failed\n");
		retVal = false;
	}

	retVal = MhlTxSendMscMsg(MHL_MSC_MSG_RCP, rcpKeyCode);
	if (retVal) {
		TX_DEBUG_PRINT("MhlTx:SiiMhlTxRcpSend\n");
		IncrementCBusReferenceCount(SiiMhlTxRcpSend)
		    MhlTxDriveStates();
	}
	return retVal;
}

/*
 * SiiMhlTxRapSend
 *
 * This function checks if the peer device supports RAP and sends rcpKeyCode. The
 * function will return a value of true if it could successfully send the RCP
 * subcommand and the key code. Otherwise false.
 */
bool_t SiiMhlTxRapSend( uint8_t rapActionCode )
{
    bool_t retVal;
    if (!(FLAGS_RCP_READY & mhlTxConfig.miscFlags)) {
        TX_DEBUG_PRINT ("MhlTx:SiiMhlTxRapSend failed\n");
        retVal = false;
    } else {
        retVal = MhlTxSendMscMsg ( MHL_MSC_MSG_RAP, rapActionCode );

        if (retVal) {
            IncrementCBusReferenceCount
            TX_DEBUG_PRINT ("MhlTx: SiiMhlTxRapSend\n");
        }
    }
    return retVal;
}

/*
 * SiiMhlTxGotMhlWriteBurst
 *
 * This function is called by the driver to inform of arrival of a scratchpad data.
 *
 * It is called in interrupt context to meet some MHL specified timings, therefore,
 * it should not have to call app layer and do negligible processing, no printfs.
 *
 * Application shall not call this function.
 */

void	SiiMhlTxGotMhlWriteBurst( uint8_t *spadArray )
{
}
#endif
