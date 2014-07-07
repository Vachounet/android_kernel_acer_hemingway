#ifndef _PN544_TEST_H_
#define _PN544_TEST_H_

#define r_ver	1
#define read_mode 4

#define nfc_pr_info(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#define nfc_pr_debug(fmt, ...)	pr_debug(fmt, ##__VA_ARGS__)

int emulation_read_mode(unsigned char *ID);
void emulation_card_mode (int status);
void Set_i2c_client (struct i2c_client *client);
int NFC_TEST(int type, unsigned char *id, int status);

#endif
