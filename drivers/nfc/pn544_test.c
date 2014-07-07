/*For pn5444 read version*/

#include <linux/completion.h>
#include <linux/crc-ccitt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/nfc/pn544.h>
#include <linux/poll.h>
#include <linux/regulator/consumer.h>
#include <linux/serial_core.h> /* for TCGETS */
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/ctype.h>
#include "nfc_codef.h"
#include "nfc_prj.h"
#include "pn544_test.h"

extern int pn544_enable(struct pn544_info *info, int mode);
extern void pn544_disable(struct pn544_info *info);
extern int gauge_device_type(void);

#define CMD_BUF_MAX_LEN        10
#define REG_ADDR_MAX_LEN	2

#define FWDL_OP_TAG_READ 	0
#define FWDL_OP_TAG_WRITE	1
#define FWDL_OP_TAG_DELAY	2

const int order = 16;
const unsigned long polynom = 0x1021;
const int direct = 1;
const unsigned long crcinit = 0xffff;
const unsigned long crcxor = 0xffff;//0x0000;
const int refin = 1;//0;
const int refout = 1;//0;

unsigned long crcmask = 0xffff;
unsigned long crchighbit = 0x8000;
unsigned long crcinit_direct = 0xffff;
unsigned long crcinit_nondirect = 0x84cf;
struct i2c_client *pn544_client;
struct chip_data_t * S_H_version = NULL;
int iFrameCount = 0;

static int pn544_i2c_write_test(struct i2c_client *client, char *buf, int count)
{
	if (count != i2c_master_send(client, buf, count)) {
		pr_err("%s: Send reg. info error\n", __func__);
		return -1;
	}
	return 0;
}

static int pn544_i2c_read_test(struct i2c_client *client, char *buf, int count)
{
  int rc;
  struct i2c_msg msgs[] = {
    {
        .addr = client->addr,
        .flags = 0,
        .len = 1,
        .buf = buf,
    },
    {
        .addr = client->addr,
        .flags = I2C_M_RD,
        .len = count,
        .buf = buf,
    },
  };

  rc = i2c_transfer(client->adapter, msgs, 2);
  if (rc < 0) {
      pr_err("%s: error %d,read addr= 0x%x,len= %d\n", __FUNCTION__,rc,*buf,count);
      return -1;
  }
  return 0;
}

unsigned long reflect (unsigned long crc, int bitnum) {
	// reflects the lower 'bitnum' bits of 'crc'
	unsigned long i, j=1, crcout=0;

	for (i=(unsigned long)1<<(bitnum-1); i; i>>=1) {
		if (crc & i) crcout|=j;
		j<<= 1;
	}
	return (crcout);
}

unsigned long crcbitbybitfast(unsigned char* p, unsigned long len) {
	// fast bit by bit algorithm without augmented zero bytes.
	// does not use lookup table, suited for polynom orders between 1...32.
	unsigned long i, j, c, bit;
	unsigned long crc = crcinit_direct;

	for (i=0; i<len; i++) {

		c = (unsigned long)*p++;
		if (refin) c = reflect(c, 8);

		for (j=0x80; j; j>>=1) {

			bit = crc & crchighbit;
			crc<<= 1;
			if (c & j) bit^= crchighbit;
			if (bit) crc^= polynom;
		}
	}

	if (refout) crc=reflect(crc, order);
	crc^= crcxor;
	crc&= crcmask;

	return(crc);
}

static void pr_pkg(const char *func, char *msg, char *buf) {
	int i;
	nfc_pr_debug("%s: %s = { ", func, msg);
	for(i = 0; i < buf[0] + 1; i++)
		nfc_pr_debug("0x%02X \n", buf[i]);
	nfc_pr_debug("}\n");
}

static int pn544_write_test(char *buf, u8 count) {
	pr_pkg(__func__, "cmd write", buf);
	return pn544_i2c_write_test(pn544_client, buf, count);
}

static int pn544_read_test(char *buf, u8 buf_len, u16 delay) {
	char *in, *tmp;
	u8 in_len;
	int res;
	u16 crc_in, crc_gen;

	in = kzalloc(33, GFP_KERNEL);
	tmp = kzalloc(1, GFP_KERNEL);
	memset(in, 0, 33);
	memset(buf, 0, buf_len);

	// read frame length
	mdelay(delay);
	res = pn544_i2c_read_test(pn544_client, tmp, 1);
	in_len = tmp[0];

	if (res)
		pr_info("[%s]I2C read error = %d\n", __func__, res);

	if(in_len == 0x51) {
		// no data to read
		nfc_pr_debug("%s: empty frame\n", __func__);
	} else if(in_len < 3 || in_len > 32) {
		pr_err("%s: bad frame length: %d\n", __func__, in_len);
		res = -1;
	} else {
		// read frame body
		in[0] = in_len;
		res = pn544_i2c_read_test(pn544_client, in + 1, in_len);

		// check CRC
		crc_in  = (((u16)in[in_len])<<8) | in[in_len-1];
		crc_gen = crcbitbybitfast(in, in_len - 1);
		if(crc_in != crc_gen) {
			pr_err("%s: bad CRC\n", __func__);
			pr_pkg(__func__, "frame CRC error", in);
			// TODO: error handling
			res = -1;
		} else {
			pr_pkg(__func__, "frame read", in);

			// copy frame data
			if(buf_len > in[0])
				memcpy(buf, in, in[0] + 1);
			else
				memcpy(buf, in, buf_len);
		}
	}
	kfree(in);
	kfree(tmp);
	return res;
}

static void pn544_gen_crc(char *buf, u8 len) {
	u16 crc = crcbitbybitfast(buf, len - 2);
	buf[len - 2] = (crc) & 0xff;
	buf[len - 1] = (crc >> 8) & 0xff;
}

static int pn544_U_RSET(char *cmd, u8 len) {
	char *frame, *ack;
	int res = 0;

	// write reset command
	frame = kzalloc(len + 4, GFP_KERNEL);
	frame[0] = len + 3;
	frame[1] = 0xF9;
	memcpy(frame+2, cmd, len);
	pn544_gen_crc(frame, len + 4);
	pn544_write_test(frame, len + 4);

	// get ack
	ack = kzalloc(4, GFP_KERNEL);
	pn544_read_test(ack, 4, 10);
	if(ack[0] != 0x03 || ack[1] != 0xE6) {
		pr_err("%s: bad ack! reset fail!\n", __func__);
		pr_pkg(__func__, "ack read", ack);
		res = -1;
	}

	// reset I-frame
	iFrameCount = 0;

	kfree(frame);
	kfree(ack);
	return res;
}

static void pn544_S_encode(u8 nextPId, u8 msg) {
	char *buf = kzalloc(4, GFP_KERNEL);
	buf[0] = 3;
	buf[1] = msg|nextPId;
	pn544_gen_crc(buf, 4);
	pn544_write_test(buf, 4);
	kfree(buf);
}

static u8 pn544_S_decode(char *buf) {
	if(buf[0] != 3 || (buf[1]&0xE0) != 0xC0) {
		// not a S-frame
		return 0xFF;
	}
	// TODO: check CRC
	return buf[1]&0xF8;
}


static int pn544_I(char *cmd, u8 len, char *buf, u8 buf_len) {
	char header[] = {0x80, 0x89, 0x92, 0x9B, 0xA4, 0xAD, 0xB6, 0xBF};
	char res_h[]  = {0x81, 0x8A, 0x93, 0x9C, 0xA5, 0xAE, 0xB7, 0xB8};
	//char ack[]  	= {0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC0};

	char *frame, *ack_in;//, *ack_out;
	bool read_retry;
	unsigned int retry_times = 0;
	int res = 0;

	// clean data
	memset(buf, 0, buf_len);

	// write I-frame
	frame = kzalloc(len + 4, GFP_KERNEL);
	frame[0] = len + 3;
	frame[1] = header[iFrameCount];
	memcpy(frame+2, cmd, len);
	pn544_gen_crc(frame, len + 4);
	nfc_pr_debug("%s: writing I-frame[%d]...\n", __func__, iFrameCount);
	msleep(8);
	pn544_write_test(frame, len + 4);

	// read return value
	ack_in = kzalloc(33, GFP_KERNEL);
	memset(ack_in, 0, 33);
	do {
		read_retry = false;
		msleep(6);
		pn544_read_test(ack_in, 33, 10);
		if(ack_in[0] >= 3) {
			if(ack_in[0] == 3 && (ack_in[1]&0xE0) == 0xC0) {
				// got S-frame
				if(pn544_S_decode(ack_in) == 0XC0) {
					nfc_pr_debug("%s: response OK: S(RR)\n", __func__);
					read_retry = true;
					retry_times = 5;
				} else {
					pr_err("%s: response ERROR: %d\n", __func__, (ack_in[1]>>3)&0x03);
					//pr_pkg(__func__, "I-frame write", frame);
					//pr_pkg(__func__, "ack read", ack_in);
					res = -1;
				}
			} else if((ack_in[1]&0xC0) == 0x80) {
				// got I-frame
				if(ack_in[1] != res_h[iFrameCount]) {
					pr_err("%s: bad ack!\n", __func__);
					//pr_pkg(__func__, "I-frame write", frame);
					//pr_pkg(__func__, "ack read", ack_in);
					res = -1;
				} else {
					if(ack_in[0] > 4) {
						// ack
						switch(ack_in[3]>>6) {
							case 0:
								nfc_pr_debug("%s: type: command: %d\n", __func__, ack_in[3]&0x3F);
								break;
							case 1:
								nfc_pr_debug("%s: type: event: %d\n", __func__, ack_in[3]&0x3F);
								break;
							case 2:
								nfc_pr_debug("%s: type: response: %d\n", __func__, ack_in[3]&0x3F);
								break;
							case 3:
								nfc_pr_debug("%s: type ERROR: %d\n", __func__, ack_in[3]);
								break;
						}
					}
					// set return data
					if(buf_len > ack_in[0])
						memcpy(buf, ack_in, ack_in[0] + 1);
					else
						memcpy(buf, ack_in, buf_len);
				}
				iFrameCount = ack_in[1]&0x07;
				nfc_pr_debug("%s: next: I-frame[%d]\n", __func__, iFrameCount);
				// auto-ack (S-frame)
				//ack_out = kzalloc(4, GFP_KERNEL);
				pn544_S_encode(iFrameCount, 0xC0);
				//ack_out[0] = 3;
				////ack_out[1] = ack[iFrameCount];
				//ack_out[1] = 0xC0&iFrameCount;
				//pn544_gen_crc(ack_out, 4);
				//pn544_write(ack_out, 4);
				//kfree(ack_out);
			} else {
				nfc_pr_debug("%s: UNKNOW response: not I/S frame\n", __func__);
				pr_pkg(__func__, "ack read", ack_in);
			}
		} else if (ack_in[0] == 0) {
			if (retry_times) {
				pr_info("%s : No data can readed, retry once\n", __func__);
				read_retry = true;
				retry_times--;
			}
		} else {
			nfc_pr_debug("%s: UNKNOW response: length = %d\n", __func__, ack_in[0]);
			pr_pkg(__func__, "ack read", ack_in);
		}
	} while(read_retry);

	// set iFrameCount
	//iFrameCount = (iFrameCount+1)&0x07;	// 0~7

	kfree(frame);
	kfree(ack_in);
	return res;
}

static void demoScript_init(char srcGId, char dstHId, char dstGId)
{
	char cmd1[] = {0x04, 0x00};	// Send U-RSET frame
	char cmd2[] = {0x81, 0x03};	// Open Pipe Admin
	char cmd3[] = {0x81, 0x14};	// Admin Clear All Pipe
	char cmd4[] = {0x81, 0x03};	// ReOpen Pipe Admin
	char cmd5[] = {0x81, 0x10, srcGId, dstHId, dstGId};	// Create Pipe to the gate system MNGT
	char cmd6[] = {0x82, 0x03};	// Open Pipe to the gate system MNGT

	pr_info("++ %s ++\n", __func__);
	if (-1 == pn544_U_RSET(cmd1, 2)) /* Retry to wake up PN544 in standby mode */
		pn544_U_RSET(cmd1, 2);
	pn544_I(cmd2, 2, 0, 0);
	pn544_I(cmd3, 2, 0, 0);
	pn544_I(cmd4, 2, 0, 0);
	pn544_I(cmd5, 5, 0, 0);
	pn544_I(cmd6, 2, 0, 0);
	pr_info("-- %s --\n", __func__);
}

static void demoScript_deinit(void) {
	char cmd1[] = {0x82,0x04};
	char cmd2[] = {0x81,0x11,0x02};

	pn544_I(cmd1, 2, 0, 0);
	pn544_I(cmd2, 3, 0, 0);
}

void Set_ActiveMode_Script(void) {
	char cmd1[] = {0x82, 0x3E, 0x00, 0x9E, 0xAA};
	char cmd2[] = {0x82, 0x3F, 0x00, 0x9E, 0xAA, 0x00};
	char *in;

	in = kzalloc(7, GFP_KERNEL);

	/*Change PN544 to active mode*/
	pr_info("[NFC] : set active mode\n");
	demoScript_init(0x20, 0x00, 0x90);
	pn544_I(cmd1, 5, in, 7);
	if (in[3] == 0x80)
		pr_info("[NFC] The nfc chip is in %s mode\n",in[4]?"standby":"active");
	pn544_I(cmd2, 6, in, 7);
	pn544_I(cmd1, 5, in, 7);
	if (in[3] == 0x80)
		pr_info("[NFC] The nfc chip is in %s mode\n",in[4]?"standby":"active");
	demoScript_deinit();
	kfree(in);

}

/*
 * This function emulation card mode
 * The devices will become ISO18092
 */
 void emulation_card_mode(int status)
{
	char cmd1[] ={0x82, 0x01, 0x01, 0x0F};
	char cmd2[] ={0x82, 0x01, 0x03, 0x11, 0x22, 0x66};
	char cmd3[] ={0x82, 0x02, 0x09};
	char cmd4[] ={0x82, 0x02, 0x0B};
	char cmd5[] ={0x82, 0x02, 0x0C};
	char cmd6[] ={0x82, 0x01, 0x01, 0x07};
	char *in;

	if (status == 1) {
		in = kzalloc(9, GFP_KERNEL);
		pr_info("[NFC_TEST] : Devices will become iso18092\n");
		demoScript_init(0x20, 0x00, 0x31);
		pn544_I(cmd1, 4, in, 9);
		pn544_I(cmd2, 6, in, 9);
		pn544_I(cmd3, 3, in, 9);
		pn544_I(cmd4, 3, in, 9);
		pn544_I(cmd5, 3, in, 9);
		pn544_I(cmd6, 4, in, 9);
		kfree(in);
	} else {
		demoScript_deinit();
	}
}

/*
 *This function let nxp to read mode(Mifare)
 *@ID : Return remote card ID->UID0, UID1, UID2, UID3
*/

int emulation_read_mode(unsigned char *ID)
{
	char cmd1[] ={0x82, 0x01, 0x06,0x7F};
	char cmd2[] ={0x82, 0x02, 0x06};
	char cmd3[] ={0x81, 0x10, 0x13, 0x00, 0x13};
	char cmd4[] ={0x83, 0x03};
	char cmd5[] ={0x81, 0x10, 0x20, 0x00, 0x90};
	char cmd6[] ={0x84, 0x03};
	char cmd7[] ={0x84, 0x02, 0x02};
	char cmd8[] ={0x84, 0x01, 0x02, 0xFF};
	char cmd9[] ={0x83, 0x01, 0x10, 0x00};
	char cmd10[] ={0x83, 0x50};
	char cmd11[] ={0x83, 0x02, 0x04};
	char cmd12[] ={0x83, 0x02, 0x03};
	char cmd13[] ={0x83, 0x02, 0x02};
	char *in;

	if (ID == NULL) {
		return -1;
	}
	in = kzalloc(9, GFP_KERNEL);
	pr_info("[NFC_TEST] : Read Mifare card ID\n");
	demoScript_init(0x20, 0x00, 0x94);
	pn544_I(cmd1, 4, in, 9);
	pn544_I(cmd2, 3, in, 9);
	pn544_I(cmd3, 5, in, 9);
	pn544_I(cmd4, 2, in, 9);
	pn544_I(cmd5, 5, in, 9);
	pn544_I(cmd6, 2, in, 9);
	pn544_I(cmd7, 3, in, 9);
	pn544_I(cmd8, 4, in, 9);
	pn544_I(cmd9, 4, in, 9);
	pn544_I(cmd10, 2, in, 9);
	pn544_I(cmd11, 3, in, 9);
	pn544_I(cmd12, 3, in, 9);
	pn544_I(cmd13, 3, in, 9);
	memcpy(ID, in+3, 4);
	pr_info("[NFC_TEST] The card ID is %x %x %x %x\n", ID[0], ID[1], ID[2], ID[3]);
	demoScript_deinit();
	kfree(in);
	return 0;
}

void NFC_ID(unsigned char *id)
{
	if (S_H_version == NULL) return;
	memcpy(id, S_H_version ->hw_ver, 3);
	memcpy(id+3, S_H_version->sw_ver, 3);
}

static void demoScript_readVersion(chip_data_t *chip_data) {
	char cmd3[] = {0x82, 0x02, 0x01};
	char cmd4[] = {0x82, 0x02, 0x03};
	char *in;

	if (chip_data == NULL)
		return;
	in = kzalloc(9, GFP_KERNEL);
	memset(chip_data->sw_ver, 0, 3);
	memset(chip_data->hw_ver, 0, 3);

	demoScript_init(0x05, 0x00, 0x05);
	pn544_I(cmd3, 3, in, 9);
	if(in[3] == 0x80)
		memcpy(chip_data->sw_ver, in+4, 3);
	pn544_I(cmd4, 3, in, 9);
	if(in[3] == 0x80)
		memcpy(chip_data->hw_ver, in+4, 3);
	demoScript_deinit();
	kfree(in);
}

void Set_i2c_client (struct i2c_client *client)
{
	pn544_client = client;
}

static void NFC_Show_ActiveStandbyMode(PU8 pu8CmdInBuf)
{
	U8	au8CmdQryMode[] = {0x82, 0x3E, 0x00, 0x9E, 0xAA};

	pn544_I(au8CmdQryMode, sizeof(au8CmdQryMode), pu8CmdInBuf, CMD_BUF_MAX_LEN);

	if (pu8CmdInBuf[3] == 0x80)
		pr_info("[NFC_TEST] PN544 in %s mode\n",pu8CmdInBuf[4]?"<HCI-Standby Possible>":"<HCI-Active>");

}

static void NFC_CheckSet_ActiveStandbyMode(BOOL bCheckOnly, enum nfc_mode_e eNfcMode)
{
	U8	au8CmdSetActiveMode[] = {0x82, 0x3F, 0x00, 0x9E, 0xAA, 0x00};
	U8	au8CmdSetStandbyMode[] = {0x82, 0x3F, 0x00, 0x9E, 0xAA, 0x01};
	U8	au8CmdBuf[CMD_BUF_MAX_LEN];

	pr_info("[NFC_TEST] %s() - enter\n",  __FUNCTION__);

	demoScript_init(0x20, 0x00, 0x90);

	NFC_Show_ActiveStandbyMode(au8CmdBuf);

	if (FALSE == bCheckOnly)
	{
		if (nfc_enable_hci_active == eNfcMode) /* Change PN544 to active mode */
			pn544_I(au8CmdSetActiveMode, sizeof(au8CmdSetActiveMode), au8CmdBuf, CMD_BUF_MAX_LEN);
		else if (nfc_enable_hci_standby == eNfcMode) /* Change PN544 to standby mode */
			pn544_I(au8CmdSetStandbyMode, sizeof(au8CmdSetStandbyMode), au8CmdBuf, CMD_BUF_MAX_LEN);
		else
			/* Do nothing */;

		NFC_Show_ActiveStandbyMode(au8CmdBuf);

	}

	demoScript_deinit();
}


static U8 NFC_Char2Digit(char cIn)
{
	if (isxdigit(cIn))
	{
		if (isdigit(cIn))
			return cIn - 0x30;
		else if (isupper(cIn))
			return cIn - 55;
		else 	/* (islower(cIn)) */
			return cIn - 87;
	}
	else
		return 0;

}

static void NFC_RegReadWrite(BOOL bRead, char * sContentStr)
{
	U8	au8CmdRegReadWrite[] = {0x82, 0x00, 0x00, 0x00, 0x00, 0x00};
	U8	au8CmdBuf[CMD_BUF_MAX_LEN];

	pr_info("[NFC_TEST] %s() -enter \n", __FUNCTION__);

	au8CmdRegReadWrite[3] = NFC_Char2Digit(sContentStr[0])<<4 | NFC_Char2Digit(sContentStr[1]);
	au8CmdRegReadWrite[4] = NFC_Char2Digit(sContentStr[2])<<4 | NFC_Char2Digit(sContentStr[3]);

	demoScript_init(0x20, 0x00, 0x90);

	if (TRUE == bRead) /* Reg Read */
	{
		au8CmdRegReadWrite[1] = 0x3E;

		pn544_I(au8CmdRegReadWrite, sizeof(au8CmdRegReadWrite)-1, au8CmdBuf, CMD_BUF_MAX_LEN);
	}
	else /* Reg Write */
	{
		au8CmdRegReadWrite[1] = 0x3F;
		au8CmdRegReadWrite[5] = NFC_Char2Digit(sContentStr[4])<<4 | NFC_Char2Digit(sContentStr[5]);

		pn544_I(au8CmdRegReadWrite, sizeof(au8CmdRegReadWrite), au8CmdBuf, CMD_BUF_MAX_LEN);
	}

	if (TRUE == bRead)
	{
		if (0x80 == au8CmdBuf[3])
			pr_info("[NFC_TEST] Reg Read content = 0x%02X\n", au8CmdBuf[4]);
		else
			pr_info("[NFC_TEST] Reg Read error!");
	}

	demoScript_deinit();
}

int NFC_TEST(int type, unsigned char *id, int status)
{
	int result = 1;
	pr_info("[NFC_TEST] start nfc test, its type = %d\n", type);

	switch(type) {

	case r_ver :
		if ( S_H_version == NULL)
			S_H_version = kzalloc(sizeof(*S_H_version), GFP_KERNEL);
		demoScript_readVersion(S_H_version);
		pr_info ("[NFC_TEST] The nfc HW version is [%d%d%d]\n",
				S_H_version->hw_ver[0], S_H_version->hw_ver[1], S_H_version->hw_ver[2]);
		pr_info ("[NFC_TEST] The nfc SW version is [%d%d%d]\n",
				S_H_version->sw_ver[0], S_H_version->sw_ver[1], S_H_version->sw_ver[2]);
		if (id == NULL || (sizeof(id)/sizeof(id[0])) < 6)
			result = 0;
		else
			NFC_ID(id);
		break;
	case read_mode :
		if (id == NULL || (sizeof(id)/sizeof(id[0])) < 4)
			result = -1;
		else
			emulation_read_mode(id);
		break;
	default :
		break;
	};

	pr_info("[NFC_TEST] end nfc test, its result = %d\n", result);
	return result;

}

S8	NFC_DrvOp(enum nfc_drv_op_e eNfcDrvTestItem, void * pvInOutBuf)
{
	PU8 			pu8Buf;
	S8			s8Ret = SUCCESS;
	struct chip_data_t *	ptChip_data;

	pr_info("[NFC_DrvTest] NFC_DrvOp() enter, op = 0x%02x, \'%c\'\n", eNfcDrvTestItem, eNfcDrvTestItem);

	switch(eNfcDrvTestItem)
	{
		case stop_nfc_test:		/* '0' */
				 g_ptNfcPrjInfo->bNfcTestStarted = FALSE;
			break;

		case check_mode_status:		/* '1' */
			{
				if (TRUE == g_ptNfcPrjInfo->bNfcEnabled)
				{
					if (gpio_get_value(NXP_PN544_FW_DL))
						pr_info ("[NFC_DrvTest] pn544 in <Download> mode\n");
					else
						NFC_CheckSet_ActiveStandbyMode(TRUE, nfc_none);
				}
				else /* (FALSE == g_ptPn544Info->tNfc_pri_info.bNfcEnabled) */
					pr_info ("[NFC_DrvTest] pn544 in <Disable> mode\n");
			}
			break;

		case enable_nfc_hci_mode:	/* '2' */
			{
				if (pn544_enable(g_ptPn544Info, PN544_ENABLE_HCI_MODE) < 0)
					s8Ret = FAILURE;
			}
			break;

		case enable_nfc_dl_mode:	/* '3' */
			{
				if (pn544_enable(g_ptPn544Info, PN544_ENABLE_FW_DL_MODE) < 0)
					s8Ret = FAILURE;
			}
			break;

		case disable_nfc:		/* '4' */
				pn544_disable(g_ptPn544Info);
			break;

		case set_hci_active_mode:	/* '5' */
				NFC_CheckSet_ActiveStandbyMode(FALSE, nfc_enable_hci_active);
			break;

		case set_hci_standby_mode:	/* '6' */
				NFC_CheckSet_ActiveStandbyMode(FALSE, nfc_enable_hci_standby);
			break;

		case act_emu_card_mode:		/* '7' */
				emulation_card_mode(1);
			break;

		case act_read_ver:		/* '8' */
			{
				if (pvInOutBuf == NULL)
					s8Ret = FAILURE;
				else
				{
					ptChip_data = (chip_data_t *)pvInOutBuf;

					demoScript_readVersion(ptChip_data);
					pr_info ("[NFC_DrvTest] The nfc HW version is [%d%d%d]\n",
						 ptChip_data->hw_ver[0], ptChip_data->hw_ver[1], ptChip_data->hw_ver[2]);
					pr_info ("[NFC_DrvTest] The nfc SW version is [%d%d%d]\n",
						 ptChip_data->sw_ver[0], ptChip_data->sw_ver[1], ptChip_data->sw_ver[2]);
				}
			}
			break;

		case act_read_card_id:		/* '9' */
			{
				pu8Buf = (PU8)pvInOutBuf;

				if (pu8Buf == NULL)
					s8Ret = FAILURE;
				else
					emulation_read_mode(pu8Buf);
			}
			break;

		case act_reg_read:		/* 'a' */
				NFC_RegReadWrite(TRUE, (char *)pvInOutBuf);
			break;

		case act_reg_write:		/* 'b' */
				NFC_RegReadWrite(FALSE, (char *)pvInOutBuf);
			break;

		case act_i2c_test:		/* 'c' */
#ifdef CONFIG_MACH_ACER_A12
				pr_info ("[NFC_DrvTest] I2C test, no gauge\n");
#else
				pr_info ("[NFC_DrvTest] I2C test, gauge device type = 0x%02x\n", gauge_device_type());
#endif
			break;

		default:
			/* Do nothing */
			break;
	};

	if ((enable_nfc_hci_mode == eNfcDrvTestItem) || (enable_nfc_dl_mode == eNfcDrvTestItem))
		msleep (10);
	else if (disable_nfc == eNfcDrvTestItem)
		msleep (50);
	else
		/* do nothing */;

	pr_info("[NFC_DrvTest] end of NFC_DrvOp(), result = %d\n", s8Ret);

	return s8Ret;
}
