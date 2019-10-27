#ifndef __AXP80_REGU_H_
#define __AXP80_REGU_H_

#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

enum {
	AXP80_ID_LDO1,
	AXP80_ID_LDO2,
	AXP80_ID_LDO3,
	AXP80_ID_LDO4,
	AXP80_ID_LDO5,
	AXP80_ID_LDO6,
	AXP80_ID_LDO7,
	AXP80_ID_LDO8,
	AXP80_ID_LDO9,
	AXP80_ID_LDO10,
	AXP80_ID_LDO11,
	AXP80_ID_DCDCA  = AXP_DCDC_ID_START,
	AXP80_ID_DCDCB,
	AXP80_ID_DCDCC,
	AXP80_ID_DCDCD,
	AXP80_ID_DCDCE,
	AXP80_REG_MAX,
};

/* AXP80 Regulator Registers */
#define AXP80_DCDCA        AXP80_DCAOUT_VOL
#define AXP80_DCDCB        AXP80_DCBOUT_VOL
#define AXP80_DCDCC        AXP80_DCCOUT_VOL
#define AXP80_DCDCD        AXP80_DCDOUT_VOL
#define AXP80_DCDCE        AXP80_DCEOUT_VOL

#define AXP80_ALDO1        AXP80_ALDO1OUT_VOL
#define AXP80_ALDO2        AXP80_ALDO2OUT_VOL
#define AXP80_ALDO3        AXP80_ALDO3OUT_VOL
#define AXP80_BLDO1        AXP80_BLDO1OUT_VOL
#define AXP80_BLDO2        AXP80_BLDO2OUT_VOL
#define AXP80_BLDO3        AXP80_BLDO3OUT_VOL
#define AXP80_BLDO4        AXP80_BLDO4OUT_VOL
#define AXP80_CLDO1        AXP80_CLDO1OUT_VOL
#define AXP80_CLDO2        AXP80_CLDO2OUT_VOL
#define AXP80_CLDO3        AXP80_CLDO3OUT_VOL
#define AXP80_SW           AXP80_STARTUP_SOURCE

#define AXP80_ALDO1EN      AXP80_ONOFF_CTRL1
#define AXP80_ALDO2EN      AXP80_ONOFF_CTRL1
#define AXP80_ALDO3EN      AXP80_ONOFF_CTRL1
#define AXP80_BLDO1EN      AXP80_ONOFF_CTRL2
#define AXP80_BLDO2EN      AXP80_ONOFF_CTRL2
#define AXP80_BLDO3EN      AXP80_ONOFF_CTRL2
#define AXP80_BLDO4EN      AXP80_ONOFF_CTRL2
#define AXP80_CLDO1EN      AXP80_ONOFF_CTRL2
#define AXP80_CLDO2EN      AXP80_ONOFF_CTRL2
#define AXP80_CLDO3EN      AXP80_ONOFF_CTRL2
#define AXP80_SWEN         AXP80_ONOFF_CTRL2

#define AXP80_DCDCAEN      AXP80_ONOFF_CTRL1
#define AXP80_DCDCBEN      AXP80_ONOFF_CTRL1
#define AXP80_DCDCCEN      AXP80_ONOFF_CTRL1
#define AXP80_DCDCDEN      AXP80_ONOFF_CTRL1
#define AXP80_DCDCEEN      AXP80_ONOFF_CTRL1

#endif /* __AXP80_REGU_H_ */
