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
#include "nfc_codef.h"
#include "pn544_test.h"

struct pn544_info * g_ptPn544Info 	= NULL;
struct nfc_prj_info_t * g_ptNfcPrjInfo 	= NULL;
