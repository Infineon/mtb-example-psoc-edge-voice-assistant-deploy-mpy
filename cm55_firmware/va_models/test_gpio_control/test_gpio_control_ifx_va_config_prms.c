/*****************************************************************************
* \file test_gpio_control_ifx_va_config_prms.c
*****************************************************************************
* \copyright
* Copyright 2025, Infineon Technologies.
* All rights reserved.
*****************************************************************************/

#include "test_gpio_control_ifx_va_config_prms.h"

int32_t test_gpio_control_sod_prms[] = {
	0,
	16000,
	160,
	IFX_PRE_PROCESS_IP_COMPONENT_SOD,
	2,
	400,
	16384
};

int32_t test_gpio_control_pre_proc_hpf_prms[] = {
	0,
	16000,
	160,
	IFX_PRE_PROCESS_IP_COMPONENT_HPF,
	0
};

int32_t test_gpio_control_denoise_prms[] = {
	0,
	16000,
	160,
	IFX_PRE_PROCESS_IP_COMPONENT_DENOISE,
	1,
	38
};

int32_t test_gpio_control_dfww_prms[] = {
	0,
	16000,
	160,
	IFX_POST_PROCESS_IP_COMPONENT_DFWWD,
	1,
	1000
};

int32_t test_gpio_control_dfcmd_prms[] = {
	0,
	16000,
	160,
	IFX_POST_PROCESS_IP_COMPONENT_DFCMD,
	2,
	2000,
	0
};
