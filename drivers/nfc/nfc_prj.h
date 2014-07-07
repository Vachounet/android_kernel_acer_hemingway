#ifndef _NFC_PRJ_H_
#define _NFC_PRJ_H_

#define NXP_PN544_INTR		       	(59)
#define NXP_PN544_FW_DL			(23)
#define NXP_PN544_RESET 		(68)

#define PN544_ENABLE_HCI_MODE		 (0)
#define PN544_ENABLE_FW_DL_MODE	 (1)

typedef struct chip_data_t {
	unsigned char sw_ver[3];
	unsigned char hw_ver[3];
} chip_data_t;

typedef struct nfc_prj_info_t {
	BOOL bNfcEnabled;
	BOOL bNfcTestStarted;
} nfc_prj_info_t;

enum nfc_drv_op_e {
	stop_nfc_test = 			0x30,	/* '0' */
	check_mode_status = 	0x31,	/* '1' */
	enable_nfc_hci_mode = 	0x32,	/* '2' */
	enable_nfc_dl_mode = 	0x33,	/* '3' */
	disable_nfc = 			0x34,	/* '4' */
	set_hci_active_mode = 	0x35,	/* '5' */
	set_hci_standby_mode = 0x36,	/* '6' */
	act_emu_card_mode = 	0x37,	/* '7' */
	act_read_ver =			0x38,	/* '8' */
	act_read_card_id =		0x39,	/* '9' */
	act_reg_read =			0x61,	/* 'a' */
	act_reg_write =			0x62,	/* 'b' */
	act_i2c_test =			0x63,	/* 'c' */
};

enum nfc_mode_e {
	nfc_none = 0,
	nfc_disable,
	nfc_enable_dl,
	nfc_enable_hci_active,
	nfc_enable_hci_standby,
};

extern struct pn544_info * 	g_ptPn544Info;
extern struct nfc_prj_info_t * 	g_ptNfcPrjInfo;

extern S8 NFC_DrvOp(enum nfc_drv_op_e eNfcDrvTestItem, void * pvInOutBuf);

#endif

