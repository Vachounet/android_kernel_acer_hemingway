#ifndef __MHL_PORTING_H__
#define __NHL_PORTING_H__

#include <linux/delay.h>
#include <linux/power_supply.h>

typedef bool bool_t;

#ifndef FALSE
#define FALSE false
#endif

#ifndef TRUE
#define TRUE true
#endif

#define BIT0         0x01
#define BIT1         0x02
#define BIT2         0x04
#define BIT3         0x08
#define BIT4         0x10
#define BIT5         0x20
#define BIT6         0x40
#define BIT7         0x80

#define TYPE_USB      1
#define TYPE_AC       2
#define TYPE_NO_POWER 3
#define TYPE_NONE     0

#define LOW          0
#define HIGH         1

//#define MHL_DEBUG
#define ASSERT_PUSH_PULL
//#define ENABLE_TX_DEBUG_PRINT

#define TX_INFO_PRINT(fmt, ...)			\
				pr_info("MHL: " fmt, ##__VA_ARGS__)
#if defined(MHL_DEBUG)
#define TX_DEBUG_PRINT(fmt, ...)			\
				pr_info("MHL: " fmt, ##__VA_ARGS__)

#define CBUS_DEBUG_PRINT(fmt, ...)			\
				pr_info("MHL_CBUS: " fmt, ##__VA_ARGS__)
#else
#define TX_DEBUG_PRINT(fmt, ...)
#define CBUS_DEBUG_PRINT(fmt, ...)
#endif

#define HalTimerWait(msec) mdelay(msec)
#define AppNotifyMhlDownStreamHPDStatusChange(dsHpdStatus)

extern void AppNotifyMhlEvent(unsigned char eventCode, unsigned char eventParam);

#define CHARGING_FUNC_NAME dwc3_mhl_charger_type_set
extern int setting_external_charger(void);
extern int CHARGING_FUNC_NAME(unsigned int chg_current) __attribute__((weak));
extern void mhl_enable_charging(unsigned int chg_current);
#endif
