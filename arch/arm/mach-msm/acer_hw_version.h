#ifndef ___ACER_HW_VERSION_H
#define __ACER_HW_VERSION_H
extern int acer_board_id;
extern int acer_rf_id;
extern int acer_sku_id;
extern int acer_boot_mode;
extern int acer_ddr_vendor;

// A12 BOARD ID
enum {
	HW_ID_UNKNOWN   = 0x00,
	HW_ID_EVB       = 0x01,
	HW_ID_EVT       = 0x02,
	HW_ID_DVT1_1    = 0x03,
	HW_ID_DVT1_2    = 0x04,
	HW_ID_DVT2      = 0x05,
	HW_ID_DVT3      = 0x06,
	HW_ID_DVT4      = 0x07,
	HW_ID_PVT       = 0x08,
	HW_ID_MAX       = 0xFF,
};

// A12 RF ID
enum {
	RF_ID_UNKNOWN   = 0x00,
	RF_ID_LTE       = 0x01,
	RF_ID_3G        = 0x02,
	RF_ID_1_1       = 0x03,
	RF_ID_2_0_1     = 0x04,
	RF_ID_MAX       = 0xFF,
};

// A12 SKU ID
enum {
	SKU_ID_UNKNOWN   = 0x00,
	SKU_ID_EU        = 0x01,
	SKU_ID_US        = 0x02,
	SKU_ID_MAX       = 0xFF,
};

// A12 power on reason
enum {
	NORMAL_BOOT             = 0x00,
	CHARGER_BOOT            = 0x01,
	RECOVERY_BOOT           = 0x02,
};
#endif /* __ACER_HW_VERSION_H */
