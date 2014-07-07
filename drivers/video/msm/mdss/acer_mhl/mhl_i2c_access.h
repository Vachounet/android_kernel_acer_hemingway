#ifndef __MHL_I2C_ACCESS_H__
#define __MHL_I2C_ACCESS_H__

enum {
	DEV_PAGE_TPI		= (0x72),
	DEV_PAGE_TX_L0	= (0x72),
	DEV_PAGE_TX_L1	= (0x7A),
	DEV_PAGE_TX_2		= (0x92),
	DEV_PAGE_TX_3		= (0x9A),
	DEV_PAGE_CBUS		= (0xC8),
};

/* Index into i2c client array (shifted left by 8) */
enum {
	TX_PAGE_TPI		= 0x0000, /* TPI */
	TX_PAGE_L0		= 0x0000, /* TX Legacy page 0 */
	TX_PAGE_L1		= 0x0100, /* TX Legacy page 1 */
	TX_PAGE_2		= 0x0200, /* TX page 2 */
	TX_PAGE_3		= 0x0300, /* TX page 3 */
	TX_PAGE_CBUS		= 0x0400, /* CBUS */
};

uint8_t SiiRegRead(uint16_t reg);
void SiiRegWrite(uint16_t reg, uint8_t value);
void SiiRegModify(uint16_t reg, uint8_t mask, uint8_t value);
void SiiRegWriteBlock(uint16_t reg, uint8_t *data, uint8_t len);
void SiiRegReadBlock(uint16_t reg, uint8_t *data, uint8_t len);
#endif
