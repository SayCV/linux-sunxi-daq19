#ifndef  __EK79007_PANEL_H__
#define  __EK79007_PANEL_H__

#include "panels.h"

#define EK79007_CMD_SETGAMMA0                   (0x80)
#define EK79007_CMD_SETGAMMA0_PARAM_1           (0x47)

#define EK79007_CMD_SETGAMMA1                   (0x81)
#define EK79007_CMD_SETGAMMA1_PARAM_1           (0x40)

#define EK79007_CMD_SETGAMMA2                   (0x82)
#define EK79007_CMD_SETGAMMA2_PARAM_1           (0x04)

#define EK79007_CMD_SETGAMMA3                   (0x83)
#define EK79007_CMD_SETGAMMA3_PARAM_1           (0x77)

#define EK79007_CMD_SETGAMMA4                   (0x84)
#define EK79007_CMD_SETGAMMA4_PARAM_1           (0x0f)

#define EK79007_CMD_SETGAMMA5                   (0x85)
#define EK79007_CMD_SETGAMMA5_PARAM_1           (0x70)

#define EK79007_CMD_SETGAMMA6                   (0x86)
#define EK79007_CMD_SETGAMMA6_PARAM_1           (0x70)

extern __lcd_panel_t ek79007_panel;

#endif
